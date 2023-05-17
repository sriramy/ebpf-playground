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

#include "common/log.h"
#include "common/prog.h"
#include "common/umem.h"
#include "common/xsk.h"


static int start_rx(char const *dev, char const *prog, int q, unsigned nfq)
{
#if 0
	libbpf_set_print(__pr_everything);
#endif

	unsigned int ifindex = if_nametoindex(dev);
	if (ifindex == 0)
		die("Unknown interface [%s]\n", dev);

	struct xdp_program *xdp_prog = load_xdp_program(
		prog, ifindex, XDP_MODE_NATIVE);
	if (xdp_prog == NULL)
		die("ERROR: xdp program not loaded: %s\n", strerror(errno));

	struct xsk_umem_info* uinfo = create_umem(nfq);
	struct xdp_sock *xdp_sk = create_xsk(dev, q, uinfo);
	update_xdp_map(xdp_prog, "xdp_sctp_xsks", xdp_sk->xsk);

#define RX_BATCH_SIZE      64
	uint32_t idx_rx;
	struct pollfd fds;
	for (;;) {

		D(printf("Poll enter...\n"));
		memset(&fds, 0, sizeof(fds));
		fds.fd = xsk_socket__fd(xdp_sk->xsk);
		fds.events = POLLIN;
		int rc = poll(&fds, 1, -1);
		if (rc <= 0 || rc > 1)
			continue;
		D(printf("Poll returned %d\n", rc));

		idx_rx = 0;
		rc = xsk_ring_cons__peek(&xdp_sk->rx, RX_BATCH_SIZE, &idx_rx);
		if (rc == 0)
			continue;
		D(printf("Received packets %d\n", rc));

		// Reserve space in the UMEM fill-queue to return the received buffers
		uint32_t idx;
		if (xsk_ring_prod__reserve(&uinfo->fq, rc, &idx) != rc)
			die("Failed xsk_ring_prod__reserve items=%d\n", rc);

		for (int i = 0; i < rc; i++, idx_rx++) {
			/* // Rx/Tx descriptor
			   struct xdp_desc {
			     __u64 addr;
				 __u32 len;
				 __u32 options;
			   };
			*/
			struct xdp_desc const* d = xsk_ring_cons__rx_desc(&xdp_sk->rx, idx_rx);
			D(printf("Packet received %d\n", d->len));
			if (d->len < ETH_HLEN) {
				printf("Short frame %d\n", d->len);
				continue;
			}
			uint8_t *pkt = xsk_umem__get_data(uinfo->buffer, d->addr);
			Dx(printf(
				   " addr=%llu, pkt=%p, buffer=%p (%p)\n",
				   d->addr, pkt, uinfo->buffer, uinfo->buffer+d->addr));
			struct ethhdr* h = (struct ethhdr*)pkt;
			printf(
				"Received packet; len=%d, proto 0x%04x\n",
				d->len, ntohs(h->h_proto));
			D(printf(
				  "UMEM fq; %u\n", xsk_prod_nb_free(&uinfo->fq, 0)));
			*xsk_ring_prod__fill_addr(&uinfo->fq, idx++) = d->addr;
		}

		// Release the buffers from the xsk RX queue
		xsk_ring_cons__release(&xdp_sk->rx, rc);
		// And (re)add them to the UMEM fill queue
		xsk_ring_prod__submit(&uinfo->fq, rc);
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
