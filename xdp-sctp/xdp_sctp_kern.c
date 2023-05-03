/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/if_ether.h>
#include <bpf/bpf_endian.h>

// Socket map for redirect to user-space
struct {
	__uint(type, BPF_MAP_TYPE_XSKMAP);
	__uint(key_size, sizeof(int));
	__uint(value_size, sizeof(int));
	__uint(max_entries, 4); /* Must be > nqueues for the nic */
	__uint(pinning, LIBBPF_PIN_BY_NAME); 
} xsks_map SEC(".maps");

static __always_inline int handle_ipv4(struct xdp_md *xdp)
{
	void *data_end = (void *)(long)xdp->data_end;
	void *data = (void *)(long)xdp->data;
	struct ethhdr *eth = data;
	struct iphdr *iph = data + sizeof(struct ethhdr);

	if (eth + 1 > data_end ||
	    iph + 1 > data_end)
		return XDP_DROP;

	switch (iph->protocol) {
	case IPPROTO_SCTP:
		break;
	default:
		return XDP_PASS;
	}

	return bpf_redirect_map(&xsks_map, xdp->rx_queue_index, XDP_PASS);
}

static __always_inline int handle_ipv6(struct xdp_md *xdp)
{
	void *data_end = (void *)(long)xdp->data_end;
	void *data = (void *)(long)xdp->data;
	struct ethhdr *eth = data;
	struct ipv6hdr *ip6h = data + sizeof(struct ethhdr);

	if (eth + 1 > data_end ||
	    ip6h + 1 > data_end)
		return XDP_DROP;

	switch (ip6h->nexthdr) {
	case IPPROTO_SCTP:
		break;
	default:
		return XDP_PASS;
	}

	return bpf_redirect_map(&xsks_map, xdp->rx_queue_index, XDP_PASS);
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

	if (h_proto == htons(ETH_P_IP))
		return handle_ipv4(xdp);
	else if (h_proto == htons(ETH_P_IPV6))

		return handle_ipv6(xdp);
	else
		return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
