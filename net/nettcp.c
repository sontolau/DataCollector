#include "netutil.c"

static int __ssl_verify_cb (int verify_ok, X509_STORE_CTX *ctx)
{
    return verify_ok;
}

static int TCPSSLInit (NetIO_t *io, NetAddr_t *addr)
{
    int check_peer = 0;

    OpenSSL_add_ssl_algorithms ();
    SSL_load_error_strings ();
    SSL_library_init ();

    io->ssl.ctx = SSL_CTX_new (SSLv23_method());
    if (addr->net_ssl.cert && addr->net_ssl.key) {
        if (SSL_CTX_use_certificate_file (io->ssl.ctx,
                                          addr->net_ssl.cert,
                                          SSL_FILETYPE_PEM) <= 0) {
ERR_RETURN:
            ERR_print_errors_fp (stderr);
            SSL_CTX_free (io->ssl.ctx);
            return -1;
        }

        if (SSL_CTX_use_PrivateKey_file (io->ssl.ctx, 
                                         addr->net_ssl.key,
                                         SSL_FILETYPE_PEM) <= 0) {
            goto ERR_RETURN;
        }

        if (SSL_CTX_check_private_key (io->ssl.ctx) <= 0) {
            goto ERR_RETURN;
        }

        check_peer = 1;
    } 
    
    if (addr->net_ssl.CA && SSL_CTX_load_verify_locations (io->ssl.ctx, 
                                       addr->net_ssl.CA, 
                                       NULL) <= 0) {
        goto ERR_RETURN;
    }
/*
    if (addr->net_flag & NET_F_BIND) { //for server
        SSL_CTX_set_verify (io->ssl.ctx,
                            check_peer?SSL_VERIFY_PEER:SSL_VERIFY_NONE,
                            check_peer?__ssl_verify_cb:NULL);
        SSL_CTX_set_verify_depth (io->ssl.ctx, 1);
    }
*/
    SSL_CTX_set_verify_depth (io->ssl.ctx, 1);
    return 0;
}


static int tcpCreateIO (NetIO_t *io)
{
    int flag = 1;
    NetAddr_t *info = io->addr_info;

    io->fd = socket (io->local_addr.ss.ss_family , SOCK_STREAM, 0);
    if (io->fd < 0) {
        return -1;
    }
    __set_nonblock (io->fd);
    if (info->net_flag & NET_F_SSL) {
        if (TCPSSLInit (io, info) < 0) {
            close (io->fd);
            return -1;
        }
    }

    if (info->net_flag & NET_F_BIND) {
        setsockopt (io->fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (int));
        if (bind (io->fd, 
                  (struct sockaddr*)&io->local_addr.ss, 
                  io->local_addr.sock_length) < 0 ||
            listen (io->fd, 1000) < 0) {
ERR_QUIT:
            if (info->net_flag & NET_F_SSL) {
                if (io->ssl.ssl) SSL_free (io->ssl.ssl);
                if (io->ssl.ctx) SSL_CTX_free (io->ssl.ctx);
            }
            close (io->fd);
            return -1;
        }
    } else {
        if (connect (io->fd, 
                     (struct sockaddr*)&io->local_addr.ss, 
                     io->local_addr.sock_length) < 0) {
            goto ERR_QUIT;
        }
        if (info->net_flag & NET_F_SSL) {
            if (!(io->ssl.ssl = SSL_new (io->ssl.ctx)) ||
                SSL_set_fd (io->ssl.ssl, io->fd) <= 0 ||
                SSL_connect (io->ssl.ssl) != 1) {
                goto ERR_QUIT;
            }
        }
    }

    return 0;
}

static int tcpWillAcceptRemoteIO (NetIO_t *io)
{
    if (io->addr_info->net_flag & NET_F_BIND) {
        return 1;
    }

    return 0;
}

static int tcpAcceptRemoteIO (NetIO_t *newio, const NetIO_t *io)
{
    newio->fd = accept (io->fd, 
                        (struct sockaddr*)&newio->local_addr.ss, 
                        &newio->local_addr.sock_length);
    if (newio->fd < 0) {
        return -1;
    }
    __set_nonblock (newio->fd);
    if (io->addr_info->net_flag & NET_F_SSL) {
        newio->ssl.ctx = NULL;
        newio->ssl.ssl = SSL_new (io->ssl.ctx);
        SSL_set_fd (newio->ssl.ssl, newio->fd);
        if (SSL_accept (newio->ssl.ssl) != 1) {
            close (newio->fd);
            if (newio->ssl.ssl) SSL_free (newio->ssl.ssl);
            return -1;
        }
    }

    return 0;
}

static long tcpReadFromIO (NetIO_t *io, unsigned char *buf, unsigned int szbuf)
{
    if (io->addr_info->net_flag & NET_F_SSL) {
        return SSL_read (io->ssl.ssl, buf, szbuf);
    }

    return __recv (io->fd, buf, szbuf);
}

static long tcpWriteToIO (const NetIO_t *io, const unsigned char *bytes, unsigned int len)
{
    if (io->addr_info->net_flag & NET_F_SSL) {
        return SSL_write (io->ssl.ssl, bytes, len);
    }

    return __send (io->fd, bytes, len);
}

static void tcpCloseIO (NetIO_t *io)
{
    if (io->addr_info->net_flag & NET_F_SSL) {
        if (io->ssl.ssl) {
            SSL_shutdown (io->ssl.ssl);
            SSL_free (io->ssl.ssl);
            io->ssl.ssl = NULL;
        }
        if (io->ssl.ctx) {
            SSL_CTX_free (io->ssl.ctx);
            io->ssl.ctx = NULL;
        }
    }

    close (io->fd);
}
