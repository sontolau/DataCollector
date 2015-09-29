#define COOKIE_SECRET_LENGTH 16

#if 0
static int __dtls_generate_cookie (SSL *ssl,
                      unsigned char *cookie,
                      unsigned int *cklen)
{
    static unsigned char cookie_rand[COOKIE_SECRET_LENGTH+1] = {0};
    unsigned char result[EVP_MAX_MD_SIZE] = {0};

    unsigned char     *buffer = NULL;
    int               buflen  = 0;
    union {
        struct sockaddr_storage ss;
        struct sockaddr_in6     s6;
        struct sockaddr_in      s4;
        struct sockaddr_un      su;
    }peer;
   
    if (!cookie_rand[0]) {
        RAND_bytes (cookie_rand, COOKIE_SECRET_LENGTH);
    }

    (void)BIO_dgram_get_peer (SSL_get_rbio (ssl), &peer);
    buffer = (unsigned char*)OPENSSL_malloc (sizeof (peer));
    if (!buffer) return 0;

    switch (peer.ss.ss_family) {
        case AF_INET:
            memcpy (buffer, &peer.s4.sin_port, 2);
            memcpy (buffer+2, &peer.s4.sin_addr, sizeof (struct in_addr));
            buflen = 2 + sizeof (struct in_addr);
            break;
        case AF_INET6:
            memcpy (buffer, &peer.s6.sin6_port, 2);
            memcpy (buffer+2, &peer.s6.sin6_addr, sizeof (struct in6_addr));
            buflen = 2 + sizeof (struct in6_addr);
         case AF_UNIX:
            strcpy ((char*)buffer, peer.su.sun_path); 
            buflen = strlen (peer.su.sun_path);
            break;
        default:
            break;
    }

    HMAC (EVP_sha1(), cookie_rand, COOKIE_SECRET_LENGTH, buffer, buflen,result, cklen);
    OPENSSL_free (buffer);

    memcpy (cookie, result, *cklen);

    return 1;
}

static int __dtls_verify_cookie (SSL *ssl, unsigned char *cookie, unsigned int cklen)
{
    unsigned char new_cookie[EVP_MAX_MD_SIZE] = {0};
    unsigned int    new_cklen = 0;

    __dtls_generate_cookie (ssl, new_cookie, &new_cklen);
    if (cklen == new_cklen && !memcmp (cookie, new_cookie, new_cklen)) {
        return 1;
    }

    return 0;
}
#endif

static int __dtls_x509_init (NetIO_t *io, int server)
{
#if 0
    int ret;
    gnutls_dh_params_t dh_parms = NULL;

    if (server) {
       dh_parms = __generate_dh_params ();
    }

    io->ssl.x509_cred = __generate_x509_cred (io->inet_addr.addr_info->ssl.ca,
                                              io->inet_addr.addr_info->ssl.cert,
                                              io->inet_addr.addr_info->ssl.key,
                                              io->inet_addr.addr_info->ssl.crl,
                                              dh_parms);
    if (io->ssl.x509_cred == NULL) {
        return -1;
    }

    if (server) {
        gnutls_priority_init(&io->ssl.priority_cache,
                             "PERFORMANCE:-VERS-TLS-ALL:+VERS-DTLS1.0:%SERVER_PRECEDENCE",
                             NULL);
        gnutls_key_generate (&io->ssl.cookie_key, GNUTLS_COOKIE_KEY_SIZE);
    } else {
        gnutls_certificate_set_verify_function (io->ssl.x509_cred,
                                                __verify_callback);
    }
#endif
    return 0;
}

static void __dtls_x509_destroy (NetIO_t *io)
{
    if (io->ssl.session) {
        gnutls_bye (io->ssl.session, GNUTLS_SHUT_RDWR);
        gnutls_deinit (io->ssl.session);
    }

    if (io->ssl.x509_cred) {
        gnutls_certificate_free_credentials (io->ssl.x509_cred);
    }

    if (io->ssl.priority_cache) {
        gnutls_priority_deinit (io->ssl.priority_cache);
    }
}

static int __dtls_connect (NetIO_t *io)
{
#if 0
    int ret;

    gnutls_init (&io->ssl.session, GNUTLS_CLIENT | GNUTLS_DATAGRAM);
    gnutls_priority_set_direct (io->ssl.session,
                                "NORMAL",
                                NULL);
    gnutls_credentials_set(io->ssl.session, 
                           GNUTLS_CRD_CERTIFICATE, 
                           io->ssl.x509_cred);
    gnutls_transport_set_int (io->ssl.session, io->fd);
    do {
        ret = gnutls_handshake(session);
    }while (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN);

    if (ret < 0) {
        gnutls_deinit (io->ssl.session);
        return -1;
    }
#endif 

    return 0;
}

static int __dtls_accept (NetIO_t *io)
{
#if 0
    int ret;
    char tmpbuf[1024] = {0};
    
    ret = __recvfrom (io->fd,
                      tmpbuf,
                      sizeof (tmpbuf),
                      MSG_PEEK,
                      (struct sockaddr*)&io->inet_addr.addr,
                      &io->inet_addr.addrlen);
    if (ret < 0) return -1;
/*
    ret = gnutls_dtls_cookie_verify (&io->ssl.cookie_key,
                                     &io->inet_addr.addr,
                                     io->inet_addr.addrlen,
                                     tmpbuf,
                                     ret,
                                     &io->ssl.prestate);
    if (ret < 0) {
        
    }
*/
    gnutls_init (&io->ssl.session, GNUTLS_SERVER | GNUTLS_DATAGRAM);
    gnutls_priority_set (io->ssl.session, io->ssl.priority_cache);
    gnutls_credentials_set (io->ssl.session, 
                            GNUTLS_CRD_CERTIFICATE,
                            io->ssl.x509_cred);
#endif

    return 0;    
}

static int udpOpenIO (NetIO_t *io, int serv)
{
    struct sockaddr *sa = (struct sockaddr*)&io->inet_addr.addr;
    int family = sa->sa_family;
    
    io->fd = socket (family, SOCK_DGRAM, 0);
    if (io->fd < 0) {
        return -1;
    }

    if (io->inet_addr.addr_info->flag & NET_F_SSL) {
        if (__dtls_x509_init (io, serv) < 0) {
            close (io->fd);
            return -1;
        }
    }

    return 0;
}


static long udpCtrlIO (NetIO_t *io, int type, void *arg, long len)
{
    INetAddress_t *sa = (INetAddress_t*)&io->inet_addr;
    NetAddrInfo_t *addr_info = sa->addr_info;
    NetSockOption_t *option;
    int i;
    NetBuf_t  *buf = (NetBuf_t*)arg;

    switch (type) {
        case NET_IO_CTRL_BIND:
        {
            return bind (io->fd, 
                         (struct sockaddr*)&sa->addr,
                         sa->addrlen);
        }
            break;
        case NET_IO_CTRL_CONNECT:
        {
            if (addr_info->flag & NET_F_SSL) {
                if (connect (io->fd,
                         (struct sockaddr*)&sa->addr,
                         sa->addrlen) < 0) {
                    return -1;
                }

                return __dtls_connect (io);
            }
 
            return 0;
        }
            break;
        case NET_IO_CTRL_LISTEN:
            return 0;
            break;
            
        case NET_IO_CTRL_ACCEPT:
            if (addr_info->flag & NET_F_SSL) {
                __dtls_accept (io);
            }
            return 0;
            break;
            
       case NET_IO_CTRL_SET_OPTS: {
            for (i=0; i<len; i++) {
                option = &((NetSockOption_t*)arg)[i];
                if (setsockopt (io->fd, 
                                option->level, 
                                option->name, 
                                option->optval, 
                                option->optlen) < 0) {
                    return -1;
                }
            }
            return 0;
        }
            break;
        case NET_IO_CTRL_GET_OPTS: {
            for (i=0; i<len; i++) {
                option = &((NetSockOption_t*)arg)[i];
                if (getsockopt (io->fd,
                                option->level,
                                option->name,
                                option->optval,
                                &option->optlen) < 0) {
                    return -1;
                }
            }
        }
            break;
        case NET_IO_CTRL_READ: {
            buf->inet_address.addrlen = sizeof (buf->inet_address.addr);
            if (addr_info->flag & NET_F_SSL) {
                ;//return SSL_read (io->ssl.ssl, arg, len);
                return gnutls_record_recv (io->ssl.session, 
                                           buf->data, 
                                           buf->size);
            }
            return __recvfrom (io->fd,
                               buf->data,
                               buf->size,
                               (struct sockaddr*)&buf->inet_address.addr,
                               &buf->inet_address.addrlen);
        }
            break;
        case NET_IO_CTRL_WRITE: {
            if (addr_info->flag & NET_F_SSL) {
               return gnutls_record_send (io->ssl.session, 
                                          buf->data, 
                                          buf->size);
            }
            return __sendto (io->fd,
                             buf->data,
                             buf->size,
                             (struct sockaddr*)&buf->inet_address.addr,
                             buf->inet_address.addrlen);
        }
            break;
    
        default:
            return -1;
    }

    return -1;
}

static void udpCloseIO (NetIO_t *io)
{
    if (io->inet_addr.addr_info->flag & NET_F_SSL) {
        __dtls_x509_destroy (io);
    }

    close (io->fd);
}
