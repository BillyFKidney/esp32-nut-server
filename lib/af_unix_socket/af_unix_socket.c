// AF_UNIX to LWIP loopback

//#include "af_unix_socket.h"

#include <net/if.h>
#include "lwip/sockets.h"

#define SERVER_PORT 3333

int af_unix_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);
    
    socklen_t addr_len = sizeof(server_addr);

    return lwip_accept(s, (struct sockaddr *)&server_addr, &addr_len);
}

int af_unix_bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    return lwip_bind(s, (struct sockaddr *)&server_addr, sizeof(server_addr));
}

int af_unix_close(int s)
{
    return lwip_close(s);
}

int af_unix_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr.sin_port = htons(SERVER_PORT);
    //inet_aton("127.0.0.1", &server_addr.sin_addr);

    return lwip_connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr));
}

int af_unix_listen(int s, int backlog)
{
    return lwip_listen(s, backlog);
}

int af_unix_socket(int domain, int type, int protocol)
{
    if (domain != 1 /*AF_UNIX*/ || type != 1 /*SOCK_STREAM*/ || protocol != 0)
    {
        // Currently, we only support AF_UNIX with SOCK_STREAM and protocol 0
        return -1; // Return an error code for unsupported domain
    }

    return lwip_socket(AF_INET, type, protocol);
}

