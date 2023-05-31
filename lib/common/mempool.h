/* 
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/

#ifndef __COMMON_MEMPOOL_H__
#define __COMMON_MEMPOOL_H__

#include <stdint.h>

#include <xdp/libxdp.h>
#include <xdp/xsk.h>

#define NUM_PORTS 10

struct mempool_block {
        uint64_t *blocks;
        bool used;
};

struct mempool_params {
        uint32_t block_nr;
        uint32_t frames_per_block;
        uint32_t frame_sz;
};

struct mempool {
	struct mempool_params params;

	uint32_t fill_size;
	uint32_t comp_size;

	void *addr;
	uint64_t *frames;
        uint64_t *frames_empty;
        struct mempool_block *prod;
        struct mempool_block *cons;

	struct xsk_ring_prod fq;
	struct xsk_ring_cons cq;
	struct xsk_umem *umem;
};

static const struct mempool_params mempool_params_default = {
        .block_nr = NUM_PORTS * 2,
        .frames_per_block = XSK_RING_PROD__DEFAULT_NUM_DESCS,
        .frame_sz = XSK_UMEM__DEFAULT_FRAME_SIZE,
};

static const struct xsk_umem_config umem_config_default = {
        .fill_size = XSK_RING_PROD__DEFAULT_NUM_DESCS * 2,
        .comp_size = XSK_RING_CONS__DEFAULT_NUM_DESCS,
        .frame_size = XSK_UMEM__DEFAULT_FRAME_SIZE,
        .frame_headroom = XSK_UMEM__DEFAULT_FRAME_HEADROOM,
        .flags = 0,
};

struct mempool *mempool_create(
	const struct mempool_params *params,
	const struct xsk_umem_config *umem_config);
void mempool_delete(struct mempool *mp);

struct mempool_block *mempool_prod_block_get(struct mempool *mp);
bool mempool_prod_block_put(struct mempool *mp, struct mempool_block *put_block);
struct mempool_block *mempool_cons_block_get(struct mempool *mp);
bool mempool_cons_block_put(struct mempool *mp, struct mempool_block *put_block);

#endif /* __COMMON_MEMPOOL_H__ */
