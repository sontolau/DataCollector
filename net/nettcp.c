#include "netutil.c"

static int __ssl_verify_cb (int verify_ok, X509_STORE_CTX *ctx)
{
    return verify_ok;
}

static int __TLS_init (NetIO_t *io, NetAddr_t *addr)
{
    int check_peer = 0;

    OpenSSL_add_ssl_algorithms ();
    SSL_load_error_strings ();
    SSL_library_init ();

    io->ssl.ctx = SSL_CTX_new (SSLv23_method());
    if (addr->net_ssl.cert && addr->net_ssl.key) {
        if (SSL_CTX_use_certificate_file (io->ssl.ctx,
                                          addr->net_ssl.cert,
                                          SSL_FILETYPE_PEM) <= 0 ||
            SSL_CTX_use_PrivateKey_file (io->ssl.ctx,
                                         addr->net_ssl.key,
                                         SSL_FILETYPE_PEM) <= 0 ||
            SSL_CTX_check_private_key (io->ssl.ctx) <= 0) {
            ERR_print_errors_fp (stderr);
            SSL_CTX_free (io->ssl.ctx);
            return -1;
        }

        check_peer = 1;
    } 
    
    if (addr->net_ssl.CA && SSL_CTX_load_verify_locations (io->ssl.ctx, 
                                       addr->net_ssl.CA, 
                                       NULL) <= 0) {
        ERR_print_errors_fp (stderr);
        SSL_CTX_free (io->ssl.ctx);
        return -1;
    }

    SSL_CTX_set_verify_depth (io->ssl.ctx, 1);
    io->ssl.ssl = SSL_new (io->ssl.ctx);
    SSL_set_fd (io->ssl.ssl, io->fd);

    return 0;
}

static void __TLS_destroy (NetIO_t *io)
{
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



static int tcpCreateIO (NetIO_t *io)
{
    NetAddr_t *info = io->addr_info;

    io->fd = socket (io->local_addr.ss.ss_family , SOCK_STREAM, 0);
    if (io->fd < 0) {
        return -1;
    }
    if (info->net_flag & NET_F_SSL) {
        if (__TLS_init (io, info) < 0) {
            close (io->fd);
            return -1;
        }
    }

    return 0;
}

static int tcpCtrlIO (NetIO_t *io, int type, void *arg, int len)
{
    switch (type) {
        case NET_IO_CTRL_BIND:
        {
            if (bind (io->fd, 
                      arg?arg:(struct sockaddr*)&io->local_addr.ss,
                      arg?len:io->local_addr.sock_length) < 0 ||
                listen (io->fd, 1000) < 0) {
                return -1;
            }
            return 0;
        }
            break;
        case NET_IO_CTRL_CONNECT:
        {
            if (connect (io->fd, 
                         arg?arg:(struct sockaddr*)&io->local_addr.ss,
                         arg?len:io->local_addr.sock_length) < 0) {
                return -1;
            }

            if (io->addr_info->net_flag & NET_F_SSL) {
                if (SSL_connect (io->ssl.ssl) != 1) {
                    return -1;
                }
            }

            return 0;
        }
            break;
        default:
            return __sock_ctrl (io->fd, type, arg, len);
    }

    return -1;
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

    if (io->addr_info->net_flag & NET_F_SSL) {
        newio->ssl.ctx = NULL;
        newio->ssl.ssl = SSL_new (io->ssl.ctx);
        SSL_set_fd (newio->ssl.ssl, newio->fd);
        if (SSL_accept (newio->ssl.ssl) != 1) {
            ERR_print_errors_fp (stderr);
            NetIOClose (newio);
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
        __TLS_destroy (io);
    }

    close (io->fd);
}
