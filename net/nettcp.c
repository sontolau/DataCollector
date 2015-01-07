#include "netutil.c"

static int tcpCreateIO (NetIO_t *io, const NetInfo_t *info, NetConfig_t *cfg)
{
    int flag = 1;
    struct sockaddr  addrinfo;

    __net2addr (info, &addrinfo);

    io->io_fd = socket ((io->io_net.net_port == 0?AF_UNIX:AF_INET), SOCK_STREAM, 0);
    if (io->io_fd < 0) {
        return -1;
    }

    if (info->net_flag & NET_IO_BIND) {
        setsockopt (io->io_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (int));
        __set_nonblock (io->io_fd);
        if (bind (io->io_fd, &addrinfo, sizeof (struct sockaddr)) < 0 ||
            listen (io->io_fd, 1000) < 0) {
            close (io->io_fd);
            return -1;
        }
    } else {
        if (connect (io->io_fd, &addrinfo, sizeof (addrinfo)) < 0) {
            close (io->io_fd);
            return -1;
        }
    }
    return 0;
}

static int tcpAcceptRemoteIO (NetIO_t *newio, const NetIO_t *io)
{
    struct sockaddr sockaddr;
    socklen_t       sklen = sizeof (sockaddr);

    newio->io_fd = accept (io->io_fd, &sockaddr, &sklen);
    if (newio->io_fd < 0) {
        return -1;
    }

    __addr2net (&sockaddr, &newio->io_net);
    return 0;
}

static double tcpReadFromIO (NetIO_t *io, unsigned char *bytes, unsigned int szbuf)
{
    
    return __recv (io->io_fd, bytes, szbuf);
}

static double tcpWriteToIO (const NetIO_t *io, const unsigned char *bytes, unsigned int length)
{
    return __send (io->io_fd, bytes, length);
}

static void tcpDestroyIO (NetIO_t *io)
{
    close (io->io_fd);
}
