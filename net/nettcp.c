#include "netutil.c"

static int tcpCreateIO (struct _Net *net, NetIO_t *io, const NetInfo_t *info)
{
    int flag = 1;
    struct sockaddr  addrinfo;

    __net2addr (info, &addrinfo);

    io->io_fd = socket ((io->io_net.net_port == 0?AF_UNIX:AF_INET), SOCK_STREAM, 0);
    if (io->io_fd < 0) {
        return -1;
    }

    setsockopt (io->io_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (int));
    __set_nonblock (io->io_fd);
    if (bind (io->io_fd, &addrinfo, sizeof (struct sockaddr)) < 0 ||
        listen (io->io_fd, 1000) < 0) {
        close (io->io_fd);
        return -1;
    }
    return 0;
}

static int tcpAcceptRemoteIO (struct _Net *net, NetIO_t *newio, const NetIO_t *io)
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

static double tcpReadFromIO (struct _Net *net, NetIO_t *io, NetBuffer_t *buffer)
{
    return (buffer->buffer_length = __recv (io->io_fd, buffer->buffer, buffer->buffer_size));
}

static double tcpWriteToIO (struct _Net *net, const NetIO_t *io, NetBuffer_t *buffer)
{
    return (buffer->buffer_length = __send (io->io_fd, buffer->buffer, buffer->buffer_length));
}

static void tcpDestroyIO (struct _Net *net, NetIO_t *io)
{
    close (io->io_fd);
}
