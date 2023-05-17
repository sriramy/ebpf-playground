/* 
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <xdp/libxdp.h>

#include "log.h"
#include "prog.h"


struct xdp_program *load_xdp_program(char const *bpf_prog, int ifindex, enum xdp_attach_mode attach_mode)
{
	char errmsg[1024];
	int err;
	struct xdp_program *xdp_prog;

	xdp_prog = xdp_program__open_file(bpf_prog, NULL, NULL);
	err = libxdp_get_error(xdp_prog);
	if (err) {
		libxdp_strerror(err, errmsg, sizeof(errmsg));
		die("ERROR: program loading failed: %s\n", errmsg);
	}

	err = xdp_program__attach(xdp_prog, ifindex, attach_mode, 0);
	if (err) {
		libxdp_strerror(err, errmsg, sizeof(errmsg));
		die("ERROR: attaching program failed: %s\n", errmsg);
	}

	return xdp_prog;
}

void remove_xdp_program(struct xdp_program *xdp_prog,
		int ifindex, enum xdp_attach_mode attach_mode)
{
	int err = xdp_program__detach(xdp_prog, ifindex, attach_mode, 0);
	if (err)
		die("Could not detach XDP program. Error: %s\n", strerror(-err));
}

void update_xdp_map(struct xdp_program *xdp_prog, char const *xsk_map, struct xsk_socket *xsk)
{
	struct bpf_object *bpf_obj = xdp_program__bpf_obj(xdp_prog);
	if (bpf_obj == NULL)
		die("ERROR: bpf object not found: %s\n", strerror(errno));
	struct bpf_map *map = bpf_object__find_map_by_name(bpf_obj, xsk_map);
	int xsks_map_fd = bpf_map__fd(map);
	if (xsks_map_fd < 0)
		die("ERROR: no xsks map found: %s\n", strerror(xsks_map_fd));

	int rc = xsk_socket__update_xskmap(xsk, xsks_map_fd);
	if (rc != 0)
		die("Update of BPF map failed(%d)\n", rc);
}