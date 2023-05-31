/* 
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/

#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

#include <xdp/libxdp.h>
#include <xdp/xsk.h>

#include "log.h"
#include "mempool.h"

struct mempool *mempool_create(
	const struct mempool_params *params,
	const struct xsk_umem_config *umem_config)
{
	struct rlimit r = {RLIM_INFINITY, RLIM_INFINITY};
	uint64_t frame_nr = params->block_nr * params->frames_per_block;
	uint64_t frames_size = frame_nr * sizeof(uint64_t);
	uint64_t blocks_size = params->block_nr * sizeof(struct mempool_block);
	/* 2 blocks : full, empty, used */
	uint64_t total_size = sizeof(struct mempool) + 2 * frames_size + 2 * blocks_size;
	uint8_t *p = NULL;
	struct mempool *mp = NULL;

	if (setrlimit(RLIMIT_MEMLOCK, &r))
		goto err;

	p = calloc(total_size, sizeof(uint8_t));
	if (!p)
		goto err;

	mp = (struct mempool*) p;
	memcpy(&mp->params, params, sizeof(mp->params));
	mp->fill_size = umem_config->fill_size;
	mp->comp_size = umem_config->comp_size;

	mp->frames = (uint64_t *)&mp[sizeof(struct mempool)];
	mp->frames_empty = (uint64_t *)&mp->frames[frames_size];
	mp->prod->blocks = (uint64_t *)&mp->frames_empty[frames_size];
	mp->prod->used = false;
	mp->cons->blocks = (uint64_t *)&mp->prod->blocks[blocks_size];
	mp->cons->used = false;
	for (uint32_t i = 0; i < frame_nr; i++)
		mp->frames[i] = i * params->frame_sz;
	for (uint32_t i = 0; i < params->block_nr; i++)
		mp->prod->blocks[i] = mp->frames[i * params->frames_per_block];
	for (uint32_t i = 0; i < params->block_nr; i++)
		mp->cons->blocks[i] = mp->frames_empty[i * params->frames_per_block];

#ifdef MEMPOOL_USE_MMAP
	D(printf("mmap\n"));
	mp->addr = mmap(NULL,
			frame_nr * params->frame_sz,
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS,
			-1, 0);
	if (mp->addr == MAP_FAILED)
		goto err;
	D(printf("mmap success\n"));
#else
	D(printf("posix_memalign\n"));
	if (posix_memalign(&mp->addr, getpagesize(),
			frame_nr * params->frame_sz) != 0)
		goto err;
	D(printf("posix_memalign success\n"));
#endif

	int rc = xsk_umem__create(&mp->umem,
				  mp->addr,
				  frame_nr * params->frame_sz,
				  &mp->fq,
				  &mp->cq,
				  umem_config);
	if (rc)
		goto err;

	return mp;

err:
	if (mp && mp->addr)
		munmap(mp->addr, frame_nr * params->frame_sz);
	if (p)
		free(p);
	return NULL;
}

void mempool_delete(struct mempool *mp)
{
	if (!mp)
		return;

	xsk_umem__delete(mp->umem);
	munmap(mp->addr, 
		mp->params.block_nr * mp->params.frames_per_block * mp->params.frame_sz);
	free(mp);
}

struct mempool_block *mempool_prod_block_get(struct mempool *mp)
{
	struct mempool_block *block = NULL;
	for (uint32_t i = 0; i < mp->params.block_nr; i++) {
		block = &mp->prod[i];
		if (!block->used) {
			block->used = true;
			break;
		}
	}

	return block;
}

bool mempool_prod_block_put(struct mempool *mp, struct mempool_block *put_block)
{
	struct mempool_block *block = NULL;
	for (uint32_t i = 0; i < mp->params.block_nr; i++) {
		block = &mp->prod[i];
		if (block == put_block) {
			block->used = false;
			return true;
		}
	}

	return false;
}

struct mempool_block *mempool_cons_block_get(struct mempool *mp)
{
	struct mempool_block *block = NULL;
	for (uint32_t i = 0; i < mp->params.block_nr; i++) {
		block = &mp->cons[i];
		if (!block->used) {
			block->used = true;
			break;
		}
	}

	return block;
}

bool mempool_cons_block_put(struct mempool *mp, struct mempool_block *put_block)
{
	struct mempool_block *block = NULL;
	for (uint32_t i = 0; i < mp->params.block_nr; i++) {
		block = &mp->cons[i];
		if (block == put_block) {
			block->used = false;
			return true;
		}
	}

	return false;
}