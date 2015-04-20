#include "netutil.c"

#define COOKIE_SECRET_LENGTH 16

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

static int __DTLS_init (NetIO_t *io, const NetAddr_t *addr)
{
    int verify_flag = 0;
    BIO   *bio = NULL;
    
    SSL_library_init ();
    OpenSSL_add_ssl_algorithms ();
    SSL_load_error_strings ();

    io->ssl.ctx = SSL_CTX_new (DTLSv1_method ());
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

        verify_flag = 1;
    }

    if (addr->net_flag & NET_F_BIND) {
        SSL_CTX_set_read_ahead (io->ssl.ctx, 1);
        SSL_CTX_set_cookie_generate_cb (io->ssl.ctx, __dtls_generate_cookie);
        SSL_CTX_set_cookie_verify_cb   (io->ssl.ctx, __dtls_verify_cookie);
    } else {
        SSL_CTX_set_verify_depth (io->ssl.ctx, 2);
        SSL_CTX_set_read_ahead (io->ssl.ctx, 1);
    }

    io->ssl.ssl = SSL_new (io->ssl.ctx);
    bio         = BIO_new_dgram (io->fd, BIO_NOCLOSE);
    SSL_set_bio (io->ssl.ssl, bio, bio);
    
    return 0;
}

static void __DTLS_destroy (NetIO_t *io)
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

static int __DTLS_connect (NetIO_t *io)
{
   BIO *bio = NULL;

    if (connect (io->fd,
                 (struct sockaddr*)&io->local_addr.ss,
                 io->local_addr.sock_length) < 0) {
       return -1;
    }

    bio = SSL_get_rbio (io->ssl.ssl);
    BIO_ctrl (bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &io->local_addr.ss);
    if (SSL_connect (io->ssl.ssl) != 1) {
        return -1;
    }

    return 0;
}

static int udpCreateIO (NetIO_t *io)
{
    NetAddr_t *info = io->addr_info;

    io->fd = socket (io->local_addr.ss.ss_family, SOCK_DGRAM, 0);
    if (io->fd < 0) {
        return -1;
    }

    if (info->net_flag & NET_F_SSL) {
        if (__DTLS_init (io, info) < 0) {
            close (io->fd);
            return -1;
        }
    }

    return 0;
}


static int udpCtrlIO (NetIO_t *io, int type, void *arg, int size)
{
    BIO *bio = NULL;

    switch (type) {
        case NET_IO_CTRL_BIND:
        {
            return bind (io->fd, 
                         arg?arg:(struct sockaddr*)&io->local_addr.ss,
                         arg?size:io->local_addr.sock_length);
        }
            break;
        case NET_IO_CTRL_CONNECT:
        {
            if (io->addr_info->net_flag & NET_F_SSL) {
                return __DTLS_connect (io);
            }
            return 0;
        }
            break;
        case NET_IO_CTRL_SET_NONBLOCK:
        {
            __set_nonblock (io->fd);
            if (io->addr_info->net_flag & NET_F_SSL) {
                bio = SSL_get_rbio (io->ssl.ssl);
                BIO_set_nbio (bio, 1);
            }
        }
            break;
        case NET_IO_CTRL_SET_RECV_TIMEOUT:
        {
            if (io->addr_info->net_flag & NET_F_SSL) {
                BIO_ctrl (SSL_get_rbio (io->ssl.ssl), BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, arg);
            }
            return __sock_ctrl (io->fd, type ,arg, size);
        }
            break;
        default:
            return __sock_ctrl (io->fd, type, arg, size);
    }

    return -1;
}

static int udpWillAcceptRemoteIO (NetIO_t *srvio)
{
    if ((srvio->addr_info->net_flag & NET_F_SSL) &&
        (srvio->addr_info->net_flag & NET_F_BIND)) {
        return 1;
    }

    return 0;
}

static int udpAcceptRemoteIO (NetIO_t *newio, const NetIO_t *srvio)
{
    int flag = 1;
    BIO  *bio = NULL;
    struct timeval timeout = {0, 0};

    bio = BIO_new_dgram (srvio->fd, BIO_NOCLOSE);
    
    BIO_ctrl (SSL_get_rbio (srvio->ssl.ssl), BIO_CTRL_DGRAM_GET_RECV_TIMEOUT, 0, &timeout);
    BIO_ctrl (bio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);
    newio->ssl.ctx = NULL;
    newio->ssl.ssl = SSL_new (srvio->ssl.ctx);

    SSL_set_bio (newio->ssl.ssl, bio, bio);
    SSL_set_options (newio->ssl.ssl, SSL_OP_COOKIE_EXCHANGE);

    if (DTLSv1_listen (newio->ssl.ssl, &newio->local_addr.ss) <=0) {
        ERR_print_errors_fp (stderr);
        return -1;
    }

    newio->fd = socket (srvio->local_addr.ss.ss_family, SOCK_DGRAM, 0);
    if (newio->fd < 0) {
        return -1;
    }

    if (udpCtrlIO (newio, NET_IO_CTRL_REUSEADDR, &flag, sizeof (flag)) < 0 ||
        bind (newio->fd, 
              (struct sockaddr*)&srvio->local_addr.ss,
              srvio->local_addr.sock_length) < 0 ||
        connect(newio->fd, 
                (struct sockaddr*)&newio->local_addr.ss,
                newio->local_addr.sock_length) < 0) {
        close (newio->fd);
        return -1;
    }
 
    BIO_set_fd (bio, newio->fd, BIO_NOCLOSE);
    BIO_ctrl (bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &newio->local_addr.ss);
    if (SSL_accept (newio->ssl.ssl) <= 0) {
        ERR_print_errors_fp (stderr);
        __DTLS_destroy (newio);
        close (newio->fd);
        return -1;
    }

    return 0;
}

//static long udpReadFromIO (NetIO_t *io, unsigned char *buf, unsigned int szbuf)
static long udpReadFromIO (NetIO_t *io, NetBuffer_t *buf)
{
    long szread = 0;

    if (io->addr_info->net_port == 0) {
        szread = __recvfrom (io->fd, 
                             buf->buffer,
                             buf->buffer_size, 
                             (struct sockaddr*)&buf->buffer_addr.su, 
                             &buf->buffer_addr.sock_length);
    } else {
        if (io->addr_info->net_flag & NET_F_SSL) {
            szread = __ssl_read (io->ssl.ssl, buf->buffer, buf->buffer_size);
        } else if (io->addr_info->net_flag & NET_F_IPv6) {
            szread = __recvfrom (io->fd, 
                                 buf->buffer,
                                 buf->buffer_size,
                                 (struct sockaddr*)&buf->buffer_addr.s6, 
                                 &buf->buffer_addr.sock_length);
        } else {
            szread = __recvfrom (io->fd, 
                                 buf->buffer, 
                                 buf->buffer_size, 
                                 (struct sockaddr*)&buf->buffer_addr.s4, 
                                 &buf->buffer_addr.sock_length);
        }
    }

    return (buf->buffer_length = szread);
}

static long udpWriteToIO (NetIO_t *io,const NetBuffer_t *buf)
{
    double szsend = 0;

    if (io->addr_info->net_port == 0) {
       szsend = __sendto (io->fd, 
                          buf->buffer,
                          buf->buffer_length, 
                          (struct sockaddr*)&buf->buffer_addr.su, 
                          buf->buffer_addr.sock_length); 
    } else {
        if (io->addr_info->net_flag & NET_F_SSL) {
            szsend = __ssl_write (io->ssl.ssl, buf->buffer, buf->buffer_length);
        } else if (io->addr_info->net_flag & NET_F_IPv6) {
            szsend = __sendto (io->fd, 
                               buf->buffer,
                               buf->buffer_length,
                               (struct sockaddr*)&buf->buffer_addr.s6, 
                               buf->buffer_addr.sock_length);
        } else {
            szsend = __sendto (io->fd, 
                               buf->buffer,
                               buf->buffer_length, 
                               (struct sockaddr*)&buf->buffer_addr.s4, 
                               buf->buffer_addr.sock_length);
        }
    }

    return szsend;
}

static void udpCloseIO (NetIO_t *io)
{
    if (io->addr_info->net_flag & NET_F_SSL) {
        __DTLS_destroy (io);
    }

    close (io->fd);
}
