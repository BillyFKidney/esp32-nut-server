#pragma once

#include "cc.h"
//#include <net/if.h>

#define SOCK_STREAM     1
#define O_NDELAY    O_NONBLOCK /* same as O_NONBLOCK, for compatibility */

typedef u8_t sa_family_t;
typedef u32_t socklen_t;

struct sockaddr {
  u8_t        sa_len;
  sa_family_t sa_family;
  char        sa_data[14];
};

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