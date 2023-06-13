/* 
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/

#include <assert.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <xdp/libxdp.h>
#include <xdp/xsk.h>

#include "log.h"
#include "port.h"

static bool port_frame_can_get(struct port* port, struct port_block *port_block) {
	return port_block->frame_nr > 0;
}

static uint64_t port_frame_get(struct port* port, struct port_block *port_block) {
	uint64_t frame_nr = port_block->frame_nr - 1;
	uint64_t frame;

	assert(frame_nr >= 0);
	frame = port_block->mb->addr[frame_nr];
	port_block->frame_nr = frame_nr;

	return frame;
}

static bool port_frame_can_put(struct port* port, struct port_block *port_block) {
	return (port_block->frame_nr < port->params.mp->params.frames_per_block);
}

static void port_frame_put(struct port *port, struct port_block *port_block, uint64_t frame)
{
	uint64_t frame_nr = port_block->frame_nr + 1;

	port_block->mb->addr[frame_nr] = frame;
	port_block->frame_nr = frame_nr;
}

struct port *port_create(struct port_params *params)
{
	struct port *p = NULL;
	int rc;

	p = calloc(sizeof(struct port), 1);
	if (!p)
		goto err;

	memcpy(&p->params, params, sizeof(p->params));

	/* xsk socket. */
	rc = xsk_socket__create_shared(&p->xsk,
					params->ifname,
					params->queue,
					params->mp->umem,
					&p->rxq,
					&p->txq,
					&p->fq,
					&p->cq,
					&params->xsk_config);
	if (rc) {
		goto err;
	}

	p->prod.mb = mempool_prod_block_get(params->mp);
	p->cons.mb = mempool_cons_block_get(params->mp);
	p->prod.frame_nr = 0;
	p->cons.frame_nr = params->mp->params.frames_per_block;

	return p;

err:
	if (p->xsk) {
		mempool_cons_block_put(p->params.mp, p->cons.mb);
		mempool_prod_block_put(p->params.mp, p->prod.mb);
		xsk_socket__delete(p->xsk);
	}
	if (p)
		free(p);
	return NULL;
}


void port_delete(struct port *p)
{
	if (!p)
		return;
	if (p->xsk) {
		mempool_cons_block_put(p->params.mp, p->cons.mb);
		mempool_prod_block_put(p->params.mp, p->prod.mb);
		xsk_socket__delete(p->xsk);
	}
	free(p);
}

void port_fq_push(struct port *p, uint32_t nb_pkts)
{
	uint32_t pos;

	for ( ; ; ) {
		int status;

		status = xsk_ring_prod__reserve(&p->fq, nb_pkts, &pos);
		if (status == nb_pkts)
			break;

		if (xsk_ring_prod__needs_wakeup(&p->fq)) {
			struct pollfd pollfd = {
				.fd = xsk_socket__fd(p->xsk),
				.events = POLLIN,
			};

			poll(&pollfd, 1, 0);
		}
	}

	for (int i = 0; i < nb_pkts; i++) {
		if (!port_frame_can_get(p, &p->cons)) {
			p->cons.mb = mempool_cons_block_get(p->params.mp);
			p->cons.frame_nr = p->params.mp->params.frames_per_block;
		}
		*xsk_ring_prod__fill_addr(&p->fq, pos + i) = 
			port_frame_get(p, &p->cons);
	}

	xsk_ring_prod__submit(&p->fq, nb_pkts);
}

void port_rx_burst(struct port *p, struct pkt_burst *b)
{
	uint32_t nb_pkts, pos;

	nb_pkts = (p->cons.frame_nr > MAX_PKT_BURST) ? MAX_PKT_BURST : p->cons.frame_nr;
	nb_pkts = xsk_ring_cons__peek(&p->rxq, nb_pkts, &pos);
	if (!nb_pkts) {
		if (xsk_ring_prod__needs_wakeup(&p->fq)) {
			struct pollfd pollfd = {
				.fd = xsk_socket__fd(p->xsk),
				.events = POLLIN,
			};

			poll(&pollfd, 1, 0);
		}
	}

	for (int i = 0; i < nb_pkts; i++) {
		b->addr[i] = xsk_ring_cons__rx_desc(&p->rxq, pos + i)->addr;
		b->len[i] = xsk_ring_cons__rx_desc(&p->rxq, pos + i)->len;
		p->stats.rx_pkts++;
		p->stats.rx_bytes += b->len[i];
	}
	b->nb_pkts = nb_pkts;

	xsk_ring_cons__release(&p->rxq, nb_pkts);
}

void port_cq_pull(struct port *p, uint32_t nb_pkts)
{
	uint32_t pos;
	nb_pkts = xsk_ring_cons__peek(&p->cq, nb_pkts, &pos);

	for (int i = 0; i < nb_pkts; i++) {
		uint64_t addr = *xsk_ring_cons__comp_addr(&p->cq, pos + i);
		if (!port_frame_can_put(p, &p->prod)) {
			mempool_prod_block_put(p->params.mp, p->prod.mb);
			p->prod.mb = mempool_prod_block_get(p->params.mp);
			p->prod.frame_nr = p->params.mp->params.frames_per_block;
		}
		port_frame_put(p, &p->prod, addr);
	}

	xsk_ring_cons__release(&p->cq, nb_pkts);
}

void port_tx_burst(struct port *p, struct pkt_burst *b)
{
	uint32_t nb_pkts, pos;
	int status;

	nb_pkts = b->nb_pkts;

	for ( ; ; ) {
		status = xsk_ring_prod__reserve(&p->txq, nb_pkts, &pos);
		if (status == nb_pkts)
			break;

		if (xsk_ring_prod__needs_wakeup(&p->txq))
			sendto(xsk_socket__fd(p->xsk), NULL, 0, MSG_DONTWAIT,
			       NULL, 0);
	}

	for (int i = 0; i < nb_pkts; i++) {
		xsk_ring_prod__tx_desc(&p->txq, pos + i)->addr = b->addr[i];
		xsk_ring_prod__tx_desc(&p->txq, pos + i)->len = b->len[i];
		p->stats.tx_pkts++;
		p->stats.tx_bytes += b->len[i];
	}

	xsk_ring_prod__submit(&p->txq, nb_pkts);
	if (xsk_ring_prod__needs_wakeup(&p->txq))
		sendto(xsk_socket__fd(p->xsk), NULL, 0, MSG_DONTWAIT, NULL, 0);
}

void port_stats_print(struct port *p, FILE *file)
{
	fprintf(file, "rx_pkts\t\trx_bytes\trx_dropped\ttx_pkts\t\ttx_bytes\ttx_dropped\t\n");
	fprintf(file, "%-12lu\t%-12lu\t%-12lu\t%-12lu\t%-12lu\t%-12lu\t\n",
		p->stats.rx_pkts, p->stats.rx_bytes, p->stats.rx_dropped,
		p->stats.tx_pkts, p->stats.tx_bytes, p->stats.tx_dropped);
}
