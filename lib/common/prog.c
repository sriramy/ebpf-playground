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
