/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <netinet/in.h>
#include <bpf/bpf_endian.h>

#if DEBUG
#define Dx(fmt, ...)                                      \
    ({                                                         \
        char ____fmt[] = fmt;                                  \
        bpf_trace_printk(____fmt, sizeof(____fmt), ##__VA_ARGS__); \
    })
#else
#define Dx(fmt, ...)
#endif

// Socket map for redirect to user-space
struct {
	__uint(type, BPF_MAP_TYPE_XSKMAP);
	__uint(key_size, sizeof(int));
	__uint(value_size, sizeof(int));
	__uint(max_entries, 1); /* Must be > nqueues for the nic */
	__uint(pinning, LIBBPF_PIN_BY_NAME); 
} xdp_sctp_xsks SEC(".maps");

static __always_inline int handle_ipv4(struct xdp_md *xdp)
{
	void *data_end = (void *)(long)xdp->data_end;
	void *data = (void *)(long)xdp->data;
	struct iphdr *iph = data + sizeof(struct ethhdr);

	if (iph + 1 > data_end)
		return XDP_DROP;

	switch (iph->protocol) {
	case IPPROTO_SCTP:
		break;
	default:
		return XDP_PASS;
	}

	unsigned int rx_queue = xdp->rx_queue_index;
	int rc = bpf_redirect_map(&xdp_sctp_xsks, rx_queue, XDP_PASS);
	Dx("SCTP XDP redirect, len=%ld, rc=%d\n", (data_end - data), rc);
	return rc;
}

static __always_inline int handle_ipv6(struct xdp_md *xdp)
{
	void *data_end = (void *)(long)xdp->data_end;
	void *data = (void *)(long)xdp->data;
	struct ipv6hdr *ip6h = data + sizeof(struct ethhdr);

	if (ip6h + 1 > data_end)
		return XDP_DROP;

	switch (ip6h->nexthdr) {
	case IPPROTO_SCTP:
		break;
	default:
		return XDP_PASS;
	}

	unsigned int rx_queue = xdp->rx_queue_index;
	int rc = bpf_redirect_map(&xdp_sctp_xsks, rx_queue, XDP_PASS);
	Dx("SCTP XDP redirect, len=%ld, rc=%d\n", (data_end - data), rc);
	return rc;
}

SEC("xdp.frags")
int _xdp_sctp_redirect(struct xdp_md *xdp)
{
	void *data_end = (void *)(long)xdp->data_end;
	void *data = (void *)(long)xdp->data;
	struct ethhdr *eth = data;
	__u16 h_proto;

	if (eth + 1 > data_end)
		return XDP_DROP;

	h_proto = eth->h_proto;

	int rc = XDP_PASS;
	switch(h_proto) {
	case ETH_P_IP:
		rc = handle_ipv4(xdp);
		break;
	case ETH_P_IPV6:
		rc = handle_ipv6(xdp);
		break;
	default:
		break;
	}

	return rc;
}

char _license[] SEC("license") = "GPL";
