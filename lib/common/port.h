/* 
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/

#ifndef __COMMON_PORT_H__
#define __COMMON_PORT_H__

#include <stdint.h>

#include <net/if.h>

#include <xdp/libxdp.h>
#include <xdp/xsk.h>

#include "mempool.h"

#define MAX_PKT_BURST 32

struct port_params {
	struct xsk_socket_config xsk_config;
	struct mempool *mp;
	char const *ifname;
	uint32_t queue;
};

struct port_stats {
	uint64_t rx_pkts;
	uint64_t rx_bytes;
	uint64_t rx_dropped;
	uint64_t tx_pkts;
	uint64_t tx_bytes;
	uint64_t tx_dropped;
};

struct port_block {
	uint64_t frame_nr;
	struct mempool_block *mb;
};

struct port {
	struct port_params params;

	struct port_block prod;
	struct port_block cons;

	struct xsk_ring_cons rxq;
	struct xsk_ring_prod txq;
	struct xsk_ring_prod fq;
	struct xsk_ring_cons cq;
	struct xsk_socket *xsk;

	struct port_stats stats;
};

struct pkt_burst {
	uint64_t addr[MAX_PKT_BURST];
	uint32_t len[MAX_PKT_BURST];
	uint32_t nb_pkts;
};


struct port *port_create(struct port_params *params);
void port_delete(struct port *p);

void port_rx_burst_prep(struct port *p, uint32_t nb_pkts);
void port_rx_burst(struct port *p, struct pkt_burst *b);
void port_rx_burst_done(struct port *p, struct pkt_burst *b);

void port_tx_burst_prep(struct port *p, uint32_t nb_pkts);
void port_tx_burst(struct port *p, struct pkt_burst *b);
void port_tx_burst_done(struct port *p, struct pkt_burst *b);

#endif /* __COMMON_PORT_H__ */