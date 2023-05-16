/* 
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <xdp/libxdp.h>
#include <xdp/xsk.h>

#include "log.h"
#include "umem.h"


// XSK_RING_PROD__DEFAULT_NUM_DESCS=2048
#define NUM_FRAMES XSK_RING_PROD__DEFAULT_NUM_DESCS


struct xsk_umem_info* create_umem(unsigned nb)
{
	struct xsk_umem_info* u;
	u = calloc(1, sizeof(*u));

	uint64_t buffer_size = XSK_UMEM__DEFAULT_FRAME_SIZE * NUM_FRAMES;
	if (posix_memalign(&u->buffer, getpagesize(), buffer_size) != 0)
		die("Can't allocate buffer memory [%s]\n", strerror(errno));
	int rc = xsk_umem__create(
		&u->umem, u->buffer, buffer_size, &u->fq, &u->cq, NULL);
	if (rc != 0)
		die("Failed to create umem; %s\n", strerror(-rc));

	uint32_t idx;
	rc = xsk_ring_prod__reserve(&u->fq, nb, &idx);
	if (rc != nb)
		die("Failed xsk_ring_prod__reserve; %s\n", strerror(-rc));
	Dx(printf("UMEM fq; reserved %u, idx = %u\n", nb, idx));
	int i;
	for (i = 0; i < nb; i++, idx++) {
		*xsk_ring_prod__fill_addr(&u->fq, idx) = i * XSK_UMEM__DEFAULT_FRAME_SIZE;
	}
	xsk_ring_prod__submit(&u->fq, nb);

	return u;
}