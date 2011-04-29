#ifndef NET_H
#define NET_H
#include <sys/socket.h>
#include "compiler.h"
int _must_check socket_connect_by_name(int *fd, int socktype, const char *name);
#endif
