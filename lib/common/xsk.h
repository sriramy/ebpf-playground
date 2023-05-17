/*
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/
#ifndef __COMMON_XSK_H__
#define __COMMON_XSK_H__

#include <stdlib.h>
#include <xdp/libxdp.h>
#include <xdp/xsk.h>

#include "umem.h"

struct xdp_sock {
	struct xsk_socket *xsk;
	struct xsk_ring_cons rx;
	struct xsk_ring_prod tx;
};


struct xdp_sock *create_xsk(char const *dev, int queue_id, struct xsk_umem_info* uinfo);
void delete_xsk(struct xdp_sock *xdp_sk);

#endif /* __COMMON_XSK_H__ */