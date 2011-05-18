#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include "compiler.h"

#define INVALID_SOCKET (-1)

#define print_socket_error() \
	fprintf(stderr, "%s():%d:socket error %d (%s)\n", __func__, __LINE__, \
	errno, strerror(errno))

/* connect to a remote host
 * fd - pointer to file descriptor
 * socktype - SOCK_STREAM or SOCK_DGRAM
 * ss - socket address
 * ss_len - length of socket address
 */
static int socket_connect_by_addr(int *fd, int socktype,
		const struct sockaddr_storage *ss, socklen_t ss_len)
{
	int e;

	assert(fd != NULL);
	assert(ss != NULL);
	assert(ss_len <= sizeof *ss);

	*fd = socket(ss->ss_family, socktype, 0);
	if (*fd == INVALID_SOCKET) {
		e = errno;
		print_socket_error();
		return e;
	}
	e = fcntl(*fd, F_SETFD, FD_CLOEXEC);
	if (e)
		goto close_fd;
	/*
	e = fcntl(*fd, F_SETFL, O_NONBLOCK);
	if (e)
		goto close_fd;
	*/
	e = connect(*fd, (struct sockaddr*)ss, ss_len);
	if (e)
		goto close_fd;

	return 0;
close_fd:
	e = errno;
	print_socket_error();
	close(*fd);
	return e;
}

/* return 0 on success */
static int socket_addr(const char *name,
			struct sockaddr_storage *ss, socklen_t *ss_len)
{
	char host[strlen(name) + 1], *port;
	struct addrinfo *ai, *curr, hints[2];
	int e;
	const int socktype = SOCK_STREAM; /* TODO: support SOCK_DGRAM */

	assert(name != NULL);
	assert(ss != NULL);
	assert(ss_len != NULL);

	/* parse host/port or host:port */
	memcpy(host, name, sizeof(host));
	port = strrchr(host, '/');
	if (!port)
		port = strrchr(host, ':');
	if (!port) {
		fprintf(stderr, "missing port\n");
		return 1;
	}
	*port++ = 0;

	/* configure hints */
	memset(hints, 0, sizeof *hints);
	hints[0].ai_family = AF_INET;
	hints[0].ai_socktype = socktype;
	hints[0].ai_next = &hints[1];
	hints[1].ai_family = AF_INET6;
	hints[1].ai_socktype = socktype;
	hints[1].ai_next = NULL;
	if ((host[0] == '*' && host[1] == 0) || host[0] == 0) {
		hints[0].ai_flags |= AI_PASSIVE;
		hints[1].ai_flags |= AI_PASSIVE;
		*host = 0;
	} else {
		hints[0].ai_flags = 0;
		hints[1].ai_flags = 0;
	}

	e = getaddrinfo(*host ? host : NULL, port, hints, &ai);
	if (e) {
		fprintf(stderr, "getaddrinfo host:%s err:%s\n", host,
			gai_strerror(e));
		return 0;
	}

	/* find a good match */
	for (curr = ai; curr; curr = curr->ai_next) {
		if (curr->ai_family == AF_INET || curr->ai_family == AF_INET6) {
			if (*ss_len >= curr->ai_addrlen) {
				*ss_len = curr->ai_addrlen;
				memcpy(ss, curr->ai_addr, curr->ai_addrlen);
				freeaddrinfo(ai);
				return 0;
			}
		}
	}
	freeaddrinfo(ai);
	fprintf(stderr, "getaddrinfo():%s:no usable addresses found.\n", name);
	return 1;
}

int _must_check socket_connect_by_name(int *fd, int socktype, const char *name)
{
	struct sockaddr_storage ss;
	socklen_t ss_len;
	int e;

	ss_len = sizeof(ss);
	e = socket_addr(name, &ss, &ss_len);
	if (e)
		return e;
	return socket_connect_by_addr(fd, socktype, &ss, ss_len);
}
