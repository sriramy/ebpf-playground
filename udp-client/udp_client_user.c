/*
   SPDX-License-Identifier: MIT
   Copyright (c) Nordix Foundation
*/

#include <getopt.h>
#include <signal.h>
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

static volatile int stop = 0;

/**
 * Populate the buffer with random data.
 */
void build(uint8_t* buffer, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        buffer[i] = (rand() % 255) + 1;
    }
}

void intHandler(int dummy) {
	stop = 1;
}

int udp_client(char const *dst_addr, int port, int nb_pkts)
{
	struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
	struct timespec start, end;
	struct sockaddr_in server;
	unsigned int serverlen;
	uint64_t delta_ms;
	ssize_t bytes;
	int sockfd, i;

	signal(SIGINT, intHandler);

	build(buffer, sizeof(buffer));
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket failed");
		return EXIT_FAILURE;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
		perror("setsockopt (SO_SNDTIMEO) failed");
		return EXIT_FAILURE;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		perror("setsockopt (SO_RCVTIMEO) failed");
		return EXIT_FAILURE;
	}

	bzero((char*)&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(dst_addr);
	server.sin_port = htons(port);

	fprintf(stderr, "UDP PING (%s) %ld bytes of data.\n", dst_addr, sizeof(buffer));
	serverlen = sizeof(server);
	for (i = 1; !stop && (nb_pkts != 0); i++) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &start);

		bytes = sendto(sockfd, &buffer[0], sizeof(buffer), 0,
			(const struct sockaddr*)&server, serverlen);
		if (bytes >= 0) {
			bytes = recvfrom(sockfd, buffer, sizeof(buffer), 0,
				(struct sockaddr*)&server, &serverlen);
			if (bytes >= 0) {
				clock_gettime(CLOCK_MONOTONIC_RAW, &end);
				delta_ms = (end.tv_sec - start.tv_sec) * 1e3f +
					(end.tv_nsec - start.tv_nsec) / 1e6f;

				fprintf(stderr, "%ld bytes from %s: udp_seq=%d time=%ld ms\n",
					bytes, dst_addr, i, delta_ms);
			}
		}

		if (bytes < 0)
			fprintf(stderr, "From %s: udp_seq=%d destination host unreachable.\n", dst_addr, i);

		if (nb_pkts > 0)
			nb_pkts--;
	}

	return EXIT_SUCCESS;
}

static struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"dst", required_argument, NULL, 'd'},
	{"port", required_argument, NULL, 'p'},
	{"count", required_argument, NULL, 'c'},
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
	int count = -1;

	while ((ch = getopt_long(argc, argv, "hd:p:c:", long_options, NULL)) != -1) {
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
			break;
		case 'c':
			count = atoi(optarg);
			break;
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
	return udp_client(dst_addr, dst_port, count);
}
