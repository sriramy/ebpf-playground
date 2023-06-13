/*
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#include "common/log.h"

#define DEFAULT_PORT (5001)
uint8_t buffer[64];

/**
 * Populate the buffer with random data.
 */
void build(uint8_t* buffer, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        buffer[i] = (rand() % 255) + 1;
    }
}

int udp_client(char const *dst_addr, int port)
{
	struct timespec start, end;
	int sockfd;
	struct sockaddr_in server;

	printf("Build Data...\n");
	build(buffer, sizeof(buffer));

	printf("Configure socket...\n");
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		fprintf(stderr, "Error opening socket");
		return EXIT_FAILURE;
	}

	bzero((char*)&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(dst_addr);
	server.sin_port = htons(port);

	printf("Send UDP data...\n");
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);

	if (sendto(sockfd, &buffer[0], sizeof(buffer), 0,
			(const struct sockaddr*)&server, sizeof(server)) < 0)
	{
		fprintf(stderr, "Error in sendto()\n");
		return EXIT_FAILURE;
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
	uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 +
		(end.tv_nsec - start.tv_nsec) / 1000;

	printf("Time to send UDP message: %f[s]\n", delta_us / 1e6f);
	printf("Finished...\n");

	return EXIT_SUCCESS;
}

static struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"dst", required_argument, NULL, 'd'},
	{"port", required_argument, NULL, 'p'},
	{NULL, 0, NULL, 0}
};

static void print_usage()
{
	printf("Usage: \n");
	for(int i = 0; i < sizeof(long_options)/sizeof(struct option) - 1; i++) {
		printf("--%s (%c) arg: %s\n",
			long_options[i].name, long_options[i].val,
			long_options[i].has_arg ? "<required>" : "");
	}
	printf("\n");
}

int main(int argc, char **argv)
{
	char ch;
	char const *dst_addr = NULL;
	int dst_port = DEFAULT_PORT;

	while ((ch = getopt_long(argc, argv, "hd:p:", long_options, NULL)) != -1) {
		switch (ch)
		{
		case 'h':
			print_usage();
			return 0;
		case 'd':
			dst_addr = strdup(optarg);
			break;
		case 'p':
			dst_port = atoi(optarg);
		default:
			break;
		}
	}

	if (dst_addr == NULL) {
		print_usage();
		die("--dst is a required argument\n");
	}

	argc -= optind;
	argv += optind;
	return udp_client(dst_addr, dst_port);
}
