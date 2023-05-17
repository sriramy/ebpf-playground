/* 
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/
#ifndef __COMMON_PROG_H__
#define __COMMON_PROG_H__

#include <stdlib.h>
#include <xdp/libxdp.h>
#include <xdp/xsk.h>


struct xdp_program *load_xdp_program(char const *bpf_prog, int ifindex, enum xdp_attach_mode attach_mode);
void remove_xdp_program(struct xdp_program *xdp_prog, int ifindex, enum xdp_attach_mode attach_mode);

void update_xdp_map(struct xdp_program *xdp_prog, char const *xsk_map, struct xsk_socket *xsk);

#endif /* __COMMON_PROG_H__ */