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
        szwrite = send (sock, buf, size, MSG_NOSIGNAL);
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
/*
DC_INLINE int __get_addr_info (const NetAddrInfo_t *net, 
                                struct __sockaddr_info *addr)
{
    struct sockaddr_in *s4 = (struct sockaddr_in*)&addr->addr;
    struct sockaddr_un *su = (struct sockaddr_un*)&addr->addr;
    struct sockaddr_in6 *s6= (struct sockaddr_in6*)&addr->addr;
    socklen_t *sklen = &addr->addrlen;

    if (net->port == 0) {
        su->sun_family = AF_UNIX;
        strncpy (su->sun_path, net->address, MAX_NET_ADDR_LEN);
        *sklen = sizeof (struct sockaddr_un);
        return su->sun_family;
    } else {
        if (net->net_flag & NET_F_IPv6) {
            s6->sin6_family = AF_INET6;
            s6->sin6_port   = htons (net->port);
            if (net->address && strlen (net->address)) {
                inet_pton (AF_INET6, net->address, &s6->sin6_addr);
            } else {
                s6->sin6_addr = in6addr_any;
            }
            *sklen = sizeof (struct sockaddr_in6);
            return s6->sin6_family;
        } else {
            s4->sin_family = AF_INET;
            s4->sin_port   = htons (net->port);
            if (net->address && strlen (net->address)) {
                inet_pton (AF_INET, net->address, &s4->sin_addr);
            } else {
                s4->sin_addr.s_addr = INADDR_ANY;
            }
            *sklen = sizeof (struct sockaddr_in);
            return s4->sin_family;
        }
    }

    return AF_INET;
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
*/

DC_INLINE gnutls_dh_params_t __generate_dh_params ()
{
    gnutls_dh_params_t dparam = NULL;

    unsigned int bits = gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH,
                                        GNUTLS_SEC_PARAM_LEGACY);
    gnutls_dh_params_init (&dparam);
    gnutls_dh_params_generate2 (dparam, bits);

    return dparam;
}

DC_INLINE gnutls_certificate_credentials_t 
__generate_x509_cred (const char *ca,
                      const char *cert,
                      const char *key,
                      const char *crl,
                      gnutls_dh_params_t dhparams)
{
    gnutls_certificate_credentials_t cred;

    gnutls_certificate_allocate_credentials (&cred);
    if (ca) {
        gnutls_certificate_set_x509_trust_file(cred, 
                                               ca,
                                               GNUTLS_X509_FMT_PEM);
    }

    if (crl) {
        gnutls_certificate_set_x509_crl_file (cred,
                                              crl,
                                              GNUTLS_X509_FMT_PEM);
    }

    if (key && cert) {
        gnutls_certificate_set_x509_key_file (cred,
                                              cert,
                                              key,
                                              GNUTLS_X509_FMT_PEM);
    }

    if (dhparams) {
        gnutls_certificate_set_dh_params (cred, dhparams);
    }

    return cred;
}


DC_INLINE int __set_nonblock (int fd)
{
    int flag = 0;

    if ((flag = fcntl (fd, F_GETFL)) < 0) {
        return -1;
    }

    flag |= O_NONBLOCK;

    return fcntl (fd, F_SETFL, &flag);
}

DC_INLINE void __NOW (const char *timefmt, char buf[], int szbuf)
{
    time_t now;
    struct tm *tmptr;

    time (&now);
    tmptr = localtime (&now);

    strftime (buf, szbuf, timefmt, tmptr);
}
/*
DC_INLINE int __sock_ctrl (int fd, int level, int type, void *arg, int szarg)
{
    socklen_t len = (socklen_t)szarg;

    switch (type) {
        case NET_IO_CTRL_REUSEADDR:
            return setsockopt (fd, level, SO_REUSEADDR, arg, len);
        case NET_IO_CTRL_SET_RECV_TIMEOUT:
            return setsockopt (fd, level, SO_RCVTIMEO, arg, len);
        case NET_IO_CTRL_SET_SEND_TIMEOUT:
            return setsockopt (fd, level, SO_SNDTIMEO, arg, len);
        case NET_IO_CTRL_GET_RECV_TIMEOUT:
            return getsockopt (fd, level, SO_RCVTIMEO, arg, &len);
        case NET_IO_CTRL_GET_SEND_TIMEOUT:
            return getsockopt (fd, level, SO_SNDTIMEO, arg, &len);
        case NET_IO_CTRL_SET_RCVBUF:
            return setsockopt (fd, level, SO_RCVBUF, arg, len);
        case NET_IO_CTRL_SET_SNDBUF:
            return setsockopt (fd, level, SO_SNDBUF, arg, len);
        case NET_IO_CTRL_GET_RCVBUF:
            return getsockopt (fd, level, SO_RCVBUF, arg, &len);
        case NET_IO_CTRL_GET_SNDBUF:
            return getsockopt (fd, level, SO_SNDBUF, arg, &len);
        case NET_IO_CTRL_SET_NONBLOCK:
            return __set_nonblock (fd);
        default:
            break;
    }

    return -1;
}
*/
