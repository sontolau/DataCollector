#include "netutil.c"

static int udpCreateIO (NetIO_t *io, const NetInfo_t *info, NetConfig_t *cfg)
{
    int flag = 1;
    struct sockaddr  addrinfo;
    unsigned int szbuf = cfg->buffer_size;

    __net2addr (info, &addrinfo);

    io->io_fd = socket ((io->io_net.net_port == 0?AF_UNIX:AF_INET), SOCK_DGRAM, 0);
    if (io->io_fd < 0) {
        return -1;
    }

    setsockopt (io->io_fd, SOL_SOCKET, SO_SNDBUF, &szbuf, sizeof (int));
    setsockopt (io->io_fd, SOL_SOCKET, SO_RCVBUF, &szbuf, sizeof (int));

    if (info->net_flag & NET_IO_BIND) {
        setsockopt (io->io_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (int));
        __set_nonblock (io->io_fd);
        if (bind (io->io_fd, &addrinfo, sizeof (struct sockaddr)) < 0) {
            close (io->io_fd);
            return -1;
        }
    }
    return 0;
}

static double udpReadFromIO (NetIO_t *io, unsigned char *bytes, unsigned int szbuf)
{
    struct sockaddr addr;
    socklen_t       sklen = sizeof (addr);
    double szread = 0;

    szread = __recvfrom (io->io_fd, bytes, szbuf,&addr, &sklen);
    if (szread > 0) {
        __addr2net (&addr, &io->io_net);
    }

    return szread;
}

double udpWriteToIO (const NetIO_t *io, const unsigned char *bytes, unsigned int length)
{
    struct sockaddr addr;
    double szsend = 0;
    __net2addr (&io->io_net, &addr);
    szsend =  __sendto (io->io_fd, bytes, length,&addr, sizeof (addr));
    return szsend;
}

static void udpDestroyIO (NetIO_t *io)
{
    close (io->io_fd);
}
