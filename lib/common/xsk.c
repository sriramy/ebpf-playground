/*
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <xdp/libxdp.h>
#include <xdp/xsk.h>

#include "log.h"
#include "umem.h"
#include "xsk.h"

struct xdp_sock *create_xsk(char const *dev, int queue_id, struct xsk_umem_info* uinfo)
{
	struct xdp_sock *xdp_sk = calloc(1, sizeof(struct xdp_sock));
	struct xsk_socket_config xsk_cfg;

	memset(&xsk_cfg, 0, sizeof(xsk_cfg));
	xsk_cfg.rx_size = XSK_RING_CONS__DEFAULT_NUM_DESCS;
	xsk_cfg.tx_size = XSK_RING_PROD__DEFAULT_NUM_DESCS;
	xsk_cfg.libbpf_flags = XSK_LIBBPF_FLAGS__INHIBIT_PROG_LOAD;
	int rc = xsk_socket__create(&xdp_sk->xsk, dev, queue_id, uinfo->umem, &xdp_sk->rx, &xdp_sk->tx, &xsk_cfg);
	if (rc != 0)
		die("Failed xsk_socket__create (ingress); %s\n", strerror(-rc));
	Dx(printf("Need wakeup; %s\n", xsk_ring_prod__needs_wakeup(&tx) ? "Yes":"No"));

	return xdp_sk;
}

void delete_xsk(struct xdp_sock *xdp_sk)
{
	free(xdp_sk);
}