#ifndef _NETUTIL_H
#define _NETUTIL_H

#include "../libdc.h"

DC_CPP (extern "C" {)

DC_INLINE long __recv (int sock, unsigned char *buf, unsigned int size)
{
    long szread;

    while (1) {
        szread = recv (sock, buf, size, 0);
        if (szread < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }

    return szread;
}

DC_INLINE long __send (int sock, unsigned char *buf, unsigned int size)
{
    long szwrite;

    while (1) {
        szwrite = send (sock, buf, size, 0);
        if (szwrite < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }

    return szwrite;
}

DC_INLINE long __recvfrom (int sock, unsigned char *buf, unsigned int size,
                           struct sockaddr *addr, socklen_t *sklen)
{
    long szread;

    while (1) {
        szread = recvfrom (sock, buf, size, 0, addr, sklen);
        if (szread < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }

    return szread;
}

DC_INLINE long __sendto (int sock, unsigned char *buf, unsigned int size,
                         struct sockaddr *addr, socklen_t sklen)
{
    long szwrite;

    while (1) {
        szwrite = sendto (sock, buf, size, 0, addr, sklen);
        if (szwrite < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }

    return szwrite;
}


DC_INLINE void SetNonblockFD (int fd)
{
    int flag = 0;

    if ((flag = fcntl (fd, F_GETFL)) < 0) {
        return;
    }

    flag |= O_NONBLOCK;

    fcntl (fd, F_SETFL, &flag);
}

DC_INLINE void CloseNetIO (Server_t *srv, NetIO_t *io)
{
    close (io->io_fd);
    ev_io_stop (srv->ev_loop, &io->io_ev);
}
DC_CPP (})
#endif

