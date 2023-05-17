/* 
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/
#ifndef __COMMON_LOG_H__
#define __COMMON_LOG_H__

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#if DEBUG
#define D(x) x
#define Dx(x) x
#else
#define D(x)
#define Dx(x)
#endif

#define die(msg...) \
	do {                    \
		fprintf(stderr, msg); \
		exit(1); \
	} while(0);

static inline int __pr_everything(enum libbpf_print_level level, const char *format,
		     va_list args)
{
	return vfprintf(stderr, format, args);
}

#endif /* __COMMON_LOG_H__ */