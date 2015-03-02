#include "netutil.c"

static int __dtls_verify_callback (int ok, X509_STORE_CTX *ctx)
{
    return 1;
}

static int DTLSInit (NetIO_t *io, const NetAddr_t *addr)
{
    BIO  *bio = NULL;

    OpenSSL_add_ssl_algorithms ();
    SSL_load_error_strings ();

    if (addr->net_flag & NET_F_BIND) {
        io->ssl.ctx = SSL_CTX_new (DTLSv1_server_method ());
    } else {
        io->ssl.ctx = SSL_CTX_new (DTLSv1_client_method ());
    }

    SSL_CTX_set_cipher_list (io->ssl.ctx, "ALL:NULL:eNULL:aNULL");
    if (addr->net_flag & NET_F_BIND) {
        SSL_CTX_set_session_cache_mode (io->ssl.ctx, SSL_SESS_CACHE_OFF);
    }
    if (!SSL_CTX_use_certificate_file (io->ssl.ctx, 
                                       addr->net_ssl.cert, 
                                       SSL_FILETYPE_PEM)) {
        fprintf (stderr,"[ERROR]: no certificate found (%s).\n", addr->net_ssl.cert);
    }

    if (!SSL_CTX_use_PrivateKey_file (io->ssl.ctx, addr->net_ssl.key, SSL_FILETYPE_PEM)) {
        fprintf (stderr,"[WARN]: no private key found (%s).\n", addr->net_ssl.key);
    }

    if (!SSL_CTX_check_private_key (io->ssl.ctx)) {
        fprintf (stderr, "[WARN]: invalid private key.\n");
    }

    SSL_CTX_set_read_ahead (io->ssl.ctx, 1);
    if (addr->net_flag & NET_F_BIND) {
        SSL_CTX_set_verify (io->ssl.ctx,
                        SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE,
                        __dtls_verify_callback);
        SSL_CTX_set_cookie_generate_cb (io->ssl.ctx, NULL);
        SSL_CTX_set_cookie_verify_cb   (io->ssl.ctx, NULL);
    } else {
        SSL_CTX_set_verify_depth (io->ssl.ctx, 2);
    }
    
    bio = BIO_new_dgram (io->fd, BIO_NOCLOSE);
    io->ssl.ssl = SSL_new (io->ssl.ctx);
    SSL_set_bio (io->ssl.ssl, bio, bio);
    SSL_set_options (io->ssl.ssl, SSL_OP_COOKIE_EXCHANGE);
    
    return 0;
}

static int udpCreateIO (NetIO_t *io)
{
    int flag = 1;
    NetAddr_t *info = io->addr_info;

    io->fd = socket (io->ss.ss_family, SOCK_DGRAM, 0);
    if (io->fd < 0) {
        return -1;
    }

    __set_nonblock (io->fd);

    if (info->net_flag & NET_F_SSL) {
        if (DTLSInit (io, info) < 0) {
ERR_QUIT:
            close (io->fd);
            return -1;
        }
    }

    if (info->net_flag & NET_F_BIND) {
        setsockopt (io->fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (int));
        if (bind (io->fd, (struct sockaddr*)&io->ss, io->sock_len) < 0) {
            if (info->net_flag & NET_F_SSL) {
                SSL_free (io->ssl.ssl);
            }
            goto ERR_QUIT;
        }
    } else if (info->net_flag & NET_F_SSL) {
        if (connect (io->fd, (struct sockaddr*)&io->ss, io->sock_len) < 0) {
            SSL_free (io->ssl.ssl);
            goto ERR_QUIT;
        }
        
        BIO_ctrl (SSL_get_rbio (io->ssl.ssl), BIO_CTRL_DGRAM_SET_CONNECTED, 0, &io->ss);
        if (SSL_connect (io->ssl.ssl) < 0) {
            SSL_free (io->ssl.ssl);
            goto ERR_QUIT;
        }
    }

    return 0;
}

static int udpWillAcceptRemoteIO (NetIO_t *srvio)
{
    if (srvio->addr_info->net_flag & NET_F_SSL) {
        return 1;
    }

    return 0;
}

static int udpAcceptRemoteIO (NetIO_t *newio, const NetIO_t *srvio)
{
    int flag = 1;
    BIO  *bio = NULL;
    int ret = -1;

    while (DTLSv1_listen (srvio->ssl.ssl, &newio->ss) <=0);
    newio->fd = socket (srvio->ss.ss_family, SOCK_DGRAM, 0);
    if (newio->fd < 0) {
        return -1;
    }

    setsockopt (newio->fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (flag));
    if (bind (newio->fd, (struct sockaddr*)&srvio->ss, srvio->sock_len) < 0 ||
        connect (newio->fd, (struct sockaddr*)&newio->ss, srvio->sock_len)) {
        close (newio->fd);
        return -1;
    }

    bio = BIO_new_dgram (newio->fd, BIO_NOCLOSE);
    newio->ssl.ssl = SSL_new (srvio->ssl.ctx);
    newio->ssl.ctx = srvio->ssl.ctx;

    SSL_set_bio (newio->ssl.ssl, bio, bio);
    BIO_ctrl (bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &newio->ss);
    
    do {ret = SSL_accept (newio->ssl.ssl);} while (0);
    
    return ret;
}

static double udpReadFromIO (NetIO_t *io, unsigned char *bytes, unsigned int szbuf)
{
    double szread = 0;

    if (io->addr_info->net_port == 0) {
        szread = __recvfrom (io->fd, bytes, szbuf, (struct sockaddr*)&io->su, &io->sock_len);
    } else {
        if (io->addr_info->net_flag & NET_F_SSL) {
            if (!((SSL_get_shutdown (io->ssl.ssl) & SSL_RECEIVED_SHUTDOWN))) {
                szread = SSL_read (io->ssl.ssl, bytes, szbuf);
            }
        } else if (io->addr_info->net_flag & NET_F_IPv6) {
            szread = __recvfrom (io->fd, bytes, szbuf,(struct sockaddr*)&io->s6, &io->sock_len);
        } else {
            szread = __recvfrom (io->fd, bytes, szbuf, (struct sockaddr*)&io->s4, &io->sock_len);
        }
    }

    return szread;
}

double udpWriteToIO (const NetIO_t *io, const unsigned char *bytes, unsigned int length)
{
    double szsend = 0;

    if (io->addr_info->net_port == 0) {
       szsend = __sendto (io->fd, bytes, length, (struct sockaddr*)&io->su, io->sock_len); 
    } else {
        if (io->addr_info->net_flag & NET_F_SSL) {
            if (!((SSL_get_shutdown (io->ssl.ssl) & SSL_RECEIVED_SHUTDOWN))) {
                szsend = SSL_write (io->ssl.ssl, bytes, length);
            }
        } else if (io->addr_info->net_flag & NET_F_IPv6) {
            szsend = __sendto (io->fd, bytes, length, (struct sockaddr*)&io->s6, io->sock_len);
        } else {
            szsend = __sendto (io->fd, bytes, length, (struct sockaddr*)&io->s4, io->sock_len);
        }
    }

    return szsend;
}

static void udpDestroyIO (NetIO_t *io)
{
    if (io->addr_info->net_flag & NET_F_SSL) {
        SSL_shutdown (io->ssl.ssl);
        SSL_free (io->ssl.ssl);
    }

    close (io->fd);
}
