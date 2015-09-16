static int __verify_callback (gnutls_session_t session)
{
    return 1;
}

static int __x509_init (NetIO_t *io, int server)
{
    gnutls_dh_params_t dh_parms = NULL;

    gnutls_global_init ();
    if (server) {
       dh_parms = __generate_dh_params ();
    }

    io->ssl.x509_cred = __generate_x509_cred (io->sock_addr.addr_info->ssl.ca,
                                              io->sock_addr.addr_info->ssl.cert,
                                              io->sock_addr.addr_info->ssl.key,
                                              io->sock_addr.addr_info->ssl.crl,
                                              dh_parms);
    if (io->ssl.x509_cred == NULL) {
        return -1;
    }

    if (server) {
        gnutls_priority_init (&io->ssl.priority_cache,
                              "PERFORMANCE:%SERVER_PRECEDENCE",
                              NULL);
    } else {
        gnutls_certificate_set_verify_function (io->ssl.x509_cred,
                                            __verify_callback);
    }
    return 0;
}

static void __x509_destroy (NetIO_t *io)
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

static int __tcp_accept (const NetIO_t *io, NetIO_t *newio)
{
    int ret;

    gnutls_init (&newio->ssl.session, GNUTLS_SERVER);
    gnutls_priority_set (newio->ssl.session, io->ssl.priority_cache);
    gnutls_credentials_set (newio->ssl.session, 
                            GNUTLS_CRD_CERTIFICATE,
                            io->ssl.x509_cred);

    if (io->sock_addr.addr_info->ssl.verify_peer) {

    } else {
        gnutls_certificate_server_set_request (newio->ssl.session,
                                               GNUTLS_CERT_IGNORE);
    }
    
    newio->fd = accept (io->fd, 
                        (struct sockaddr*)&newio->sock_addr.addr,
                        &newio->sock_addr.addrlen);
    if (newio->fd < 0) {
err_quit:
        gnutls_bye (newio->ssl.session, GNUTLS_SHUT_WR);
        gnutls_deinit (newio->ssl.session);
        return -1;
    }

    gnutls_transport_set_int (newio->ssl.session, newio->fd);
    do { ret = gnutls_handshake (newio->ssl.session);} 
    while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

    if (ret < 0) {
        close (newio->fd);
        goto err_quit;
    }

    return 0;
}

static int tcpOpenIO (NetIO_t *io, int serv)
{
    NetAddrInfo_t *info = io->sock_addr.addr_info;
    struct sockaddr *sa = (struct sockaddr*)&io->sock_addr.addr;
    int family = sa->sa_family;
    
    io->fd = socket (family, SOCK_STREAM, 0);
    if (io->fd < 0) {
        return -1;
    }

    if (info->flag & NET_F_SSL) {
        if (__x509_init (io, serv) < 0) {
            close (io->fd);
            return -1;
        }
    }

    return 0;
}

static long tcpCtrlIO (NetIO_t *io, int type, void *arg, long len)
{
    NetSockAddr_t *sa = (NetSockAddr_t*)&io->sock_addr;
    NetAddrInfo_t *addr_info = sa->addr_info;
    NetSockOption_t *option;
    int             i, ret;

    switch (type) {
        case NET_IO_CTRL_BIND:
        {
            return bind (io->fd, (struct sockaddr*)&sa->addr, sa->addrlen);
        }
            break;
        case NET_IO_CTRL_LISTEN: {
            return listen (io->fd, *(int*)arg);
        }
            break;
        case NET_IO_CTRL_CONNECT:
        {
            if (connect (io->fd, 
                         (struct sockaddr*)&sa->addr,
                         sa->addrlen) < 0) {
                return -1;
            }
           
            if (addr_info->flag & NET_F_SSL) {
                gnutls_init (&io->ssl.session, GNUTLS_CLIENT);
                gnutls_set_default_priority (io->ssl.session);
                gnutls_credentials_set (io->ssl.session,
                                        GNUTLS_CRD_CERTIFICATE,
                                        io->ssl.x509_cred);
                gnutls_transport_set_int (io->ssl.session, io->fd);
                do { ret = gnutls_handshake (io->ssl.session); }
                while (ret < 0 && gnutls_error_is_fatal (ret) == 0);
                if (ret < 0) {
                    gnutls_deinit (io->ssl.session);
                    return -1;
                }
            }

            return 0;
        }
            break;
        case NET_IO_CTRL_ACCEPT: {
            return __tcp_accept (io, (NetIO_t*)arg);
        }
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
            if (addr_info->flag & NET_F_SSL) {
                return gnutls_record_recv (io->ssl.session, arg, len);
            }
            return __recv (io->fd, arg, len);
        }
            break;
        case NET_IO_CTRL_WRITE: {
            if (addr_info->flag & NET_F_SSL) {
                return gnutls_record_send (io->ssl.session, arg, len);
            }
            return __send (io->fd, arg, len);
        }
            break;
    
        default:
            return -1;
    }

    return -1;
}

static void tcpCloseIO (NetIO_t *io)
{
    NetAddrInfo_t *addr_info = io->sock_addr.addr_info;

    if (addr_info->flag & NET_F_SSL) {
        __x509_destroy (io);
    }

    close (io->fd);
}
