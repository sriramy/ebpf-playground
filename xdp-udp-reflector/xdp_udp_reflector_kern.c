/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <netinet/in.h>
#include <bpf/bpf_endian.h>

// Socket map for redirect to user-space
struct {
	__uint(type, BPF_MAP_TYPE_XSKMAP);
	__uint(key_size, sizeof(int));
	__uint(value_size, sizeof(int));
	__uint(max_entries, 1); /* Must be > nqueues for the nic */
	__uint(pinning, LIBBPF_PIN_BY_NAME);
} xdp_udp_xsks SEC(".maps");

#ifdef DEBUG
#define DEBUG_PRINTK(fmt, ...)                                      \
    ({                                                         \
        char ____fmt[] = fmt;                                  \
        bpf_trace_printk(____fmt, sizeof(____fmt), ##__VA_ARGS__); \
    })
#else
#define DEBUG_PRINTK(fmt, ...)
#endif

static __always_inline int handle_ipv4(struct xdp_md *xdp)
{
	void *data_end = (void *)(long)xdp->data_end;
	void *data = (void *)(long)xdp->data;
	struct iphdr *iph = data + sizeof(struct ethhdr);

	if (iph + 1 > data_end)
		return XDP_DROP;

	switch (iph->protocol) {
	case IPPROTO_UDP:
		break;
	default:
		return XDP_PASS;
	}

	return bpf_redirect_map(&xdp_udp_xsks, xdp->rx_queue_index, XDP_PASS);
}

static __always_inline int handle_ipv6(struct xdp_md *xdp)
{
	void *data_end = (void *)(long)xdp->data_end;
	void *data = (void *)(long)xdp->data;
	struct ipv6hdr *ip6h = data + sizeof(struct ethhdr);

	if (ip6h + 1 > data_end)
		return XDP_DROP;

	switch (ip6h->nexthdr) {
	case IPPROTO_UDP:
		break;
	default:
		return XDP_PASS;
	}

	return bpf_redirect_map(&xdp_udp_xsks, xdp->rx_queue_index, XDP_PASS);
}

SEC("xdp.frags")
int _xdp_udp_redirect(struct xdp_md *xdp)
{
	void *data_end = (void *)(long)xdp->data_end;
	void *data = (void *)(long)xdp->data;
	struct ethhdr *eth = data;

	if (eth + 1 > data_end)
		return XDP_DROP;

	int rc = XDP_PASS;
	switch(bpf_ntohs(eth->h_proto)) {
	case ETH_P_IP:
		rc = handle_ipv4(xdp);
		break;
	case ETH_P_IPV6:
		rc = handle_ipv6(xdp);
		break;
	default:
		break;
	}

	DEBUG_PRINTK("_xdp_udp_redirect len=%ld, queue=%d, rc=%d",
		(data_end - data), xdp->rx_queue_index, rc);
	return rc;
}

char _license[] SEC("license") = "GPL";
