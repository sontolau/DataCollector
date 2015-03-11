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

DC_INLINE void __net2addr (const NetAddr_t *net, NetSocketAddr_t *addr)
{
    struct sockaddr_in *s4 = (struct sockaddr_in*)&addr->s4;
    struct sockaddr_un *su = (struct sockaddr_un*)&addr->su;
    struct sockaddr_in6 *s6= (struct sockaddr_in6*)&addr->s6;
    socklen_t *sklen = &addr->sock_length;

    if (net->net_port == 0) {
        su->sun_family = AF_UNIX;
        strncpy (su->sun_path, net->net_addr, MAX_NET_ADDR_LEN);
        *sklen = sizeof (struct sockaddr_un);
    } else {
        if (net->net_flag & NET_F_IPv6) {
            s6->sin6_family = AF_INET6;
            s6->sin6_port   = htons (net->net_port);
            if (net->net_addr && strlen (net->net_addr)) {
                inet_pton (AF_INET6, net->net_addr, &s6->sin6_addr);
            } else {
                s6->sin6_addr = in6addr_any;
            }
            *sklen = sizeof (struct sockaddr_in6);
        } else {
            s4->sin_family = AF_INET;
            s4->sin_port   = htons (net->net_port);
            if (net->net_addr && strlen (net->net_addr)) {
                inet_pton (AF_INET, net->net_addr, &s4->sin_addr);
            } else {
                s4->sin_addr.s_addr = INADDR_ANY;
            }
            *sklen = sizeof (struct sockaddr_in);
        }
    }
}

DC_INLINE long __ssl_read (SSL *ssl,unsigned char *buf, unsigned int szbuf)
{
    long szread = 0;

   while (ssl) {
        if (SSL_get_shutdown (ssl) & SSL_RECEIVED_SHUTDOWN) return 0;
       szread = SSL_read (ssl, buf, szbuf);
       switch (SSL_get_error (ssl, (int)szread)) {
           case SSL_ERROR_NONE:
               return szread;
           case SSL_ERROR_WANT_READ:
           case SSL_ERROR_WANT_WRITE:
               break;
           case SSL_ERROR_ZERO_RETURN:
               return 0;
           default:
               break;
       }
   }

   return -1;
}

DC_INLINE long __ssl_write (SSL *ssl, const unsigned char *buf, unsigned int szbuf)
{
    long szwrite = 0;

    while (ssl) {
        if (SSL_get_shutdown (ssl) & SSL_RECEIVED_SHUTDOWN) return 0;

        szwrite = SSL_write (ssl, buf, szbuf);
        switch (SSL_get_error (ssl, (int)szwrite)) {
            case SSL_ERROR_NONE:
                return szwrite;
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_READ:
                break;
            case SSL_ERROR_ZERO_RETURN:
                return -0;
            default:
                break;
        }
    }

    return -1;
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

DC_INLINE void __NOW (const char *timefmt, char buf[], int szbuf)
{
    time_t now;
    struct tm *tmptr;

    time (&now);
    tmptr = localtime (&now);

    strftime (buf, szbuf, timefmt, tmptr);
}

DC_INLINE int __sock_ctrl (int fd, int type, void *arg, int szarg)
{
    socklen_t len = (socklen_t)szarg;

    switch (type) {
        case NET_IO_CTRL_REUSEADDR:
            return setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, arg, len);
        case NET_IO_CTRL_SET_RECV_TIMEOUT:
            return setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, arg, len);
        case NET_IO_CTRL_SET_SEND_TIMEOUT:
            return setsockopt (fd, SOL_SOCKET, SO_SNDTIMEO, arg, len);
        case NET_IO_CTRL_GET_RECV_TIMEOUT:
            return getsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, arg, &len);
        case NET_IO_CTRL_GET_SEND_TIMEOUT:
            return getsockopt (fd, SOL_SOCKET, SO_SNDTIMEO, arg, &len);
        case NET_IO_CTRL_SET_RCVBUF:
            return setsockopt (fd, SOL_SOCKET, SO_RCVBUF, arg, len);
        case NET_IO_CTRL_SET_SNDBUF:
            return setsockopt (fd, SOL_SOCKET, SO_SNDBUF, arg, len);
        case NET_IO_CTRL_GET_RCVBUF:
            return getsockopt (fd, SOL_SOCKET, SO_RCVBUF, arg, &len);
        case NET_IO_CTRL_GET_SNDBUF:
            return getsockopt (fd, SOL_SOCKET, SO_SNDBUF, arg, &len);
        default:
            break;
    }

    return -1;
}
#endif
