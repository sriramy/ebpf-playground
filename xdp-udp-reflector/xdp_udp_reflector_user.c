/*
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <xdp/libxdp.h>
#include <xdp/xsk.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <arpa/inet.h>
#include <time.h>

#include <xdp/libxdp.h>
#include <xdp/xsk.h>

#include "common/log.h"
#include "common/mempool.h"
#include "common/port.h"
#include "common/prog.h"

uint16_t checksum (uint16_t *addr, int len)
{
	int count = len;
	register uint32_t csum = 0;

	// Sum up 2-byte values until none or only one byte left.
	while (count > 1) {
		csum += *(addr++);
		count -= 2;
	}

	// Add left-over byte, if any.
	if (count > 0)
		csum += *(uint8_t *) addr;

	// Fold 32-bit sum into 16 bits; we lose information by doing this,
	// increasing the chances of a collision.
	// sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
	while (csum >> 16)
		csum = (csum & 0xffff) + (csum >> 16);

	// Checksum is one's compliment of sum.
	return ~csum;
}

static void udp_reflector(void *data)
{
	struct ethhdr *eth = (struct ethhdr *)data;
	struct iphdr *ip = (struct iphdr *)((void*)eth + sizeof(*eth));
	struct udphdr *udp = (struct udphdr *)((void*)ip + sizeof(*ip));

	unsigned char tmp_mac[ETH_ALEN];
	uint32_t tmp_ip;
	uint16_t tmp_port;

	/* SWAP MAC address */
	memcpy(&tmp_mac[0], &eth->h_source[0], sizeof(tmp_mac));
	memcpy(&eth->h_source[0], &eth->h_dest[0], sizeof(tmp_mac));
	memcpy(&eth->h_dest[0], &tmp_mac[0], sizeof(tmp_mac));

	/* SWAP IP address */
	tmp_ip = ip->saddr;
	ip->saddr = ip->daddr;
	ip->daddr = tmp_ip;

	/* Update IP options */
	ip->ttl = 64;
	ip->check = 0;
	ip->check = checksum((uint16_t*)ip, sizeof(*ip));

	/* SWAP UDP port */
	tmp_port = udp->source;
	udp->source = udp->dest;
	udp->dest = tmp_port;

	/* Update UDP options */
	udp->check = 0;
}

int start_rx(char const *dev, char const *prog, int q, unsigned nfq)
{
	unsigned int ifindex = if_nametoindex(dev);
	if (ifindex == 0)
		die("Unknown interface [%s]\n", dev);

	struct xdp_program *xdp_prog = xdp_prog_load(
		prog, ifindex, XDP_MODE_NATIVE);
	if (!xdp_prog)
		die("ERROR: xdp program not loaded: %s\n", strerror(errno));

	struct mempool *mp = mempool_create(&mempool_params_default, &umem_config_default);
	if (!mp)
		die("ERROR: Memory pool NOT created: %s\n", strerror(errno));

	struct port_params pparms = {
		.xsk_config.rx_size = XSK_RING_CONS__DEFAULT_NUM_DESCS,
		.xsk_config.tx_size = XSK_RING_PROD__DEFAULT_NUM_DESCS,
		.xsk_config.libxdp_flags = XSK_LIBBPF_FLAGS__INHIBIT_PROG_LOAD,
		.xsk_config.bind_flags = XDP_ZEROCOPY | XDP_USE_NEED_WAKEUP,
		.mp = mp,
		.queue = q
	};
	pparms.ifname = strdup(dev);

	struct port *p = port_create(&pparms);
	if (!p)
		die("ERROR: Port NOT created: %s\n", strerror(errno));

	xdp_map_update(xdp_prog, "xdp_udp_xsks", p->xsk);

	struct pkt_burst rx_burst;
	struct pkt_burst tx_burst;
	uint32_t nb_pkts = mp->fill_size;
	memset(&rx_burst, 0, sizeof(rx_burst));
	memset(&tx_burst, 0, sizeof(tx_burst));

	uint8_t stats_interval = 10; // 10 seconds
	time_t last = time(NULL);

	for (;;) {
		if (nb_pkts) {
			port_cq_pull(p, MAX_PKT_BURST);
			port_fq_push(p, nb_pkts);
			memset(&rx_burst, 0, sizeof(rx_burst));
			memset(&tx_burst, 0, sizeof(tx_burst));

			if ((time(NULL) - last) > (stats_interval - 1)) {
				port_stats_print(p, stderr);
				last = time(NULL);
			}
		}

		port_rx_burst(p, &rx_burst);
		nb_pkts = rx_burst.nb_pkts;

		if (!nb_pkts) {
			usleep(10 * 1000);
			continue;
		}

		for (int i = 0; i < nb_pkts; i++) {
			uint64_t addr = rx_burst.addr[i];
			uint32_t len = rx_burst.len[i];

			if (len < ETH_HLEN) {
				printf("Short frame %d\n", len);
				continue;
			}

			uint8_t *pkt = xsk_umem__get_data(mp->addr, addr);
			udp_reflector(pkt);

			tx_burst.addr[tx_burst.nb_pkts] = addr;
			tx_burst.len[tx_burst.nb_pkts] = len;
			tx_burst.nb_pkts++;
		}

		port_tx_burst(p, &tx_burst);
	}

	return EXIT_SUCCESS;
}

static struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"dev", required_argument, NULL, 'd'},
	{"queue", required_argument, NULL, 'q'},
	{"fillq", required_argument, NULL, 'f'},
	{"prog", required_argument, NULL, 'p'},
	{NULL, 0, NULL, 0}
};

static void print_usage()
{
	printf("Usage: \n");
	for(int i = 0; i < sizeof(long_options)/sizeof(struct option) - 1; i++) {
		printf("--%s (%c) arg: %s\n",
			long_options[i].name, long_options[i].val,
			long_options[i].has_arg ? "<required>" : "");
	}
	printf("\n");
}

int main(int argc, char **argv)
{
	char ch;
	char const *dev = NULL;
	char const *prog = NULL;
	int queue = 0;
	int fillq = 512;

	while ((ch = getopt_long(argc, argv, "hd:q:f:p:", long_options, NULL)) != -1) {
		switch (ch)
		{
		case 'h':
			print_usage();
			return 0;
		case 'd':
			dev = strdup(optarg);
			break;
		case 'q':
			queue = atoi(optarg);
			break;
		case 'f':
			fillq = atoi(optarg);
			break;
		case 'p':
			prog = strdup(optarg);
		default:
			break;
		}
	}

	if (dev == NULL) {
		print_usage();
		die("--dev is a required argument\n");
	} else if (prog == NULL) {
		prog = strdup("/usr/local/lib/bpf/xdp_udp_reflector_kern.o");
		printf("No program provided, using default BPF prog: %s!!!\n", prog);
	}

	argc -= optind;
	argv += optind;
	return start_rx(dev, prog, queue, fillq);
}
