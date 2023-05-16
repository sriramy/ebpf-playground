/* 
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/

#include <xdp/libxdp.h>
#include <xdp/xsk.h>

// This struct is used in xdp-tutorial and kernel sample
struct xsk_umem_info {
	struct xsk_ring_prod fq;
	struct xsk_ring_cons cq;
	struct xsk_umem *umem;
	void *buffer;
};

struct xsk_umem_info* create_umem(unsigned nb);
