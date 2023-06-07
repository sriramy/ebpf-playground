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
#include <arpa/inet.h>

#include <xdp/libxdp.h>
#include <xdp/xsk.h>

#include "common/log.h"
#include "common/mempool.h"
#include "common/port.h"
#include "common/prog.h"


static int start_rx(char const *dev, char const *prog, int q, unsigned nfq)
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
		.xsk_config.bind_flags = XDP_USE_NEED_WAKEUP,
		.mp = mp,
		.queue = q
	};
	pparms.ifname = strdup(dev);

	struct port *p = port_create(&pparms);
	if (!p)
		die("ERROR: Port NOT created: %s\n", strerror(errno));

	xdp_map_update(xdp_prog, "xdp_sctp_xsks", p->xsk);

	struct pkt_burst rx_burst;
	uint32_t nb_pkts = mp->fill_size;

	for (;;) {
		if (nb_pkts) {
			port_fq_setup(p, nb_pkts);
			memset(&rx_burst, 0, sizeof(rx_burst));
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

			D(printf("Packet received %d\n", len));
			if (len < ETH_HLEN) {
				printf("Short frame %d\n", len);
				continue;
			}
			uint8_t *pkt = xsk_umem__get_data(mp->addr, addr);
			Dx(printf(" addr=%lu, pkt=%p, buffer=%p (%p)\n",
				addr, pkt, mp->addr, mp->addr + addr));
			struct ethhdr* h = (struct ethhdr*)pkt;
			D(printf("Received packet; len=%d, proto 0x%04x\n",
				len, ntohs(h->h_proto)));
			D(printf("UMEM fq; %u\n", xsk_prod_nb_free(&mp->fq, 0)));
		}
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

void print_usage()
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
		prog = strdup("/usr/local/lib/bpf/xdp_sctp_kern.o");
		printf("No program provided, using default BPF prog: %s!!!\n", prog);
	}

	argc -= optind;
	argv += optind;
	return start_rx(dev, prog, queue, fillq);
}
