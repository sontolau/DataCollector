#ifndef _LIBPNET_H
#define _LIBPNET_H

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

DC_INLINE long __send (int sock, const unsigned char *buf, unsigned int size)
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

DC_INLINE long __sendto (int sock, const unsigned char *buf, unsigned int size,
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

DC_INLINE void __net2addr (const NetInfo_t *net, struct sockaddr *addr)
{
    if (net->net_port == 0) {
        ((struct sockaddr_un*)addr)->sun_family = AF_UNIX;
        strncpy (((struct sockaddr_un*)addr)->sun_path, net->net_path, UNIX_PATH_MAX-1);
    } else {
        ((struct sockaddr_in*)addr)->sin_family = AF_INET;
        ((struct sockaddr_in*)addr)->sin_port   = htons (net->net_port);
        ((struct sockaddr_in*)addr)->sin_addr.s_addr = net->net_ip;
    }
}

DC_INLINE void __addr2net (const struct sockaddr *addr, NetInfo_t *net)
{
    if (((struct sockaddr_in*)addr)->sin_family == AF_INET) {
        net->net_port = ntohs (((struct sockaddr_in*)addr)->sin_port);
        net->net_ip   = ((struct sockaddr_in*)addr)->sin_addr.s_addr;
    } else if (((struct sockaddr_un*)addr)->sun_family == AF_UNIX) {
        net->net_port  = 0;
        strncpy (net->net_path, ((struct sockaddr_un*)addr)->sun_path, UNIX_PATH_MAX-1);
    }
}

DC_INLINE void __set_nonblock (int fd)
{
    int flag = 0;

    if ((flag = fcntl (fd, F_GETFL)) < 0) {
        return;
    }

    flag |= O_NONBLOCK;

    fcntl (fd, F_SETFL, &flag);
}


#endif
