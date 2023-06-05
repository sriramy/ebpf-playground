/* 
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/
#ifndef __COMMON_LOG_H__
#define __COMMON_LOG_H__

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef DEBUG
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

#endif /* __COMMON_LOG_H__ */