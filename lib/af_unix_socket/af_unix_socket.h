#pragma once

#include "cc.h"
#include <sys/un.h>
//#include "../lwip/lwip/src/include/lwip/sockets.h" // for PIO
#include "../lwip/src/include/lwip/sockets.h" // for ESP-IDF

int af_unix_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int af_unix_bind(int s, const struct sockaddr *name, socklen_t namelen);
int af_unix_close(int s);
int af_unix_connect(int s, const struct sockaddr *name, socklen_t namelen);
int af_unix_listen(int s, int backlog);
int af_unix_socket(int domain, int type, int protocol);

static inline int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    return af_unix_accept(s, addr, addrlen);
}

static inline int bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    return af_unix_bind(s, name, namelen);
}

static inline int closesocket(int s)
{
    return af_unix_close(s);
}

static inline int connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    return af_unix_connect(s, name, namelen);
}

static inline int listen(int s, int backlog)
{
    return af_unix_listen(s, backlog);
}

static inline int socket(int domain, int type, int protocol)
{
    return af_unix_socket(domain, type, protocol);
}