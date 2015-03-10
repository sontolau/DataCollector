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

static int DTLSInit (NetIO_t *io, const NetAddr_t *addr)
{
    int verify_flag = 0;

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
/*
        SSL_CTX_set_verify (io->ssl.ctx,
                            verify_flag?(SSL_VERIFY_PEER|\
                                         SSL_VERIFY_FAIL_IF_NO_PEER_CERT)
                            :SSL_VERIFY_NONE,
                            __dtls_verify_cb);
*/
        SSL_CTX_set_read_ahead (io->ssl.ctx, 1);
        SSL_CTX_set_cookie_generate_cb (io->ssl.ctx, __dtls_generate_cookie);
        SSL_CTX_set_cookie_verify_cb   (io->ssl.ctx, __dtls_verify_cookie);
    } else {
        SSL_CTX_set_verify_depth (io->ssl.ctx, 2);
        SSL_CTX_set_read_ahead (io->ssl.ctx, 1);
    }


    return 0;
}

static int udpCreateIO (NetIO_t *io)
{
    int flag = 1;
    NetAddr_t *info = io->addr_info;
    BIO       *bio  = NULL;
    struct timeval timeout = {0,100};

    io->fd = socket (io->local_addr.ss.ss_family, SOCK_DGRAM, 0);
    if (io->fd < 0) {
        return -1;
    }

    __set_nonblock (io->fd);

    if (info->net_flag & NET_F_SSL) {
        if (DTLSInit (io, info) < 0) {
            close (io->fd);
            return -1;
        }
    }

    if (info->net_flag & NET_F_BIND) {
        setsockopt (io->fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (int));
        if (bind (io->fd, 
            (struct sockaddr*)&io->local_addr.ss, 
            io->local_addr.sock_length) < 0) {
ERR_QUIT:
            if (info->net_flag & NET_F_SSL) {
                SSL_free (io->ssl.ssl);
                SSL_CTX_free (io->ssl.ctx);
                ERR_print_errors_fp (stderr);
            }
            close (io->fd);
            return -1;
        }
    } else if (info->net_flag & NET_F_SSL) {
        if (connect (io->fd, 
                     (struct sockaddr*)&io->local_addr.ss, 
                     io->local_addr.sock_length) < 0) {
            goto ERR_QUIT;
        }
        bio = BIO_new_dgram (io->fd, BIO_NOCLOSE);
        BIO_ctrl (bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &io->local_addr.ss);
        BIO_ctrl (bio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);
        io->ssl.ssl = SSL_new (io->ssl.ctx);
        SSL_set_bio (io->ssl.ssl, bio, bio);
        if (SSL_connect (io->ssl.ssl) != 1) {
            goto ERR_QUIT;
        }
    }

    return 0;
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
    struct timeval timeout = {0,100};

    bio = BIO_new_dgram (srvio->fd, BIO_NOCLOSE);
    newio->ssl.ctx = NULL;
    newio->ssl.ssl = SSL_new (srvio->ssl.ctx);
    SSL_set_bio (newio->ssl.ssl, bio, bio);
    SSL_set_options (newio->ssl.ssl, SSL_OP_COOKIE_EXCHANGE);
    BIO_ctrl (bio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);

    if (DTLSv1_listen (newio->ssl.ssl, &newio->local_addr.ss) <=0) {
ERR_QUIT:
        SSL_free (newio->ssl.ssl);
        ERR_print_errors_fp (stderr);
        return -1;
    }

    newio->fd = socket (srvio->local_addr.ss.ss_family, SOCK_DGRAM, 0);
    if (newio->fd < 0) {
        goto ERR_QUIT;
    }
    __set_nonblock (newio->fd);
    setsockopt (newio->fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (flag));
    if (bind (newio->fd, 
              (struct sockaddr*)&srvio->local_addr.ss, 
              srvio->local_addr.sock_length) < 0 ||
        connect (newio->fd, 
                 (struct sockaddr*)&newio->local_addr.ss, 
                 srvio->local_addr.sock_length) < 0) {
        close (newio->fd);
        goto ERR_QUIT;
    }

    bio = SSL_get_rbio (newio->ssl.ssl);
    BIO_set_fd (bio, newio->fd, BIO_NOCLOSE);
    BIO_ctrl (bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &newio->local_addr.ss);
    BIO_ctrl (bio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);
    if (SSL_accept (newio->ssl.ssl) <= 0) {
        close (newio->fd);
        goto ERR_QUIT;
    }

    
    return 0;
}

static long udpReadFromIO (NetIO_t *io, unsigned char *buf, unsigned int szbuf)
{
    long szread = 0;

    if (io->addr_info->net_port == 0) {
        szread = __recvfrom (io->fd, 
                             buf,
                             szbuf, 
                             (struct sockaddr*)&io->local_addr.su, 
                             &io->local_addr.sock_length);
    } else {
        if (io->addr_info->net_flag & NET_F_SSL) {
            szread = __ssl_read (io->ssl.ssl, buf, szbuf);
        } else if (io->addr_info->net_flag & NET_F_IPv6) {
            szread = __recvfrom (io->fd, 
                                 buf,
                                 szbuf,
                                 (struct sockaddr*)&io->local_addr.s6, 
                                 &io->local_addr.sock_length);
        } else {
            szread = __recvfrom (io->fd, 
                                 buf, 
                                 szbuf, 
                                 (struct sockaddr*)&io->local_addr.s4, 
                                 &io->local_addr.sock_length);
        }
    }

    return szread;
}

static long udpWriteToIO (const NetIO_t *io,const unsigned char *bytes, unsigned int length)
{
    double szsend = 0;

    if (io->addr_info->net_port == 0) {
       szsend = __sendto (io->fd, 
                          bytes,
                          length, 
                          (struct sockaddr*)&io->local_addr.su, 
                          io->local_addr.sock_length); 
    } else {
        if (io->addr_info->net_flag & NET_F_SSL) {
            szsend = __ssl_write (io->ssl.ssl, bytes, length);
        } else if (io->addr_info->net_flag & NET_F_IPv6) {
            szsend = __sendto (io->fd, 
                               bytes,
                               length, 
                               (struct sockaddr*)&io->local_addr.s6, 
                               io->local_addr.sock_length);
        } else {
            szsend = __sendto (io->fd, 
                               bytes,
                               length, 
                               (struct sockaddr*)&io->local_addr.s4, 
                               io->local_addr.sock_length);
        }
    }

    return szsend;
}

static void udpCloseIO (NetIO_t *io)
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
