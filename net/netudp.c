#include "netutil.c"

static int udpCreateIO (struct _Net *net, NetIO_t *io, const NetInfo_t *info)
{
    int flag = 1;
    struct sockaddr  addrinfo;

    __net2addr (info, &addrinfo);

    io->io_fd = socket ((io->io_net.net_port == 0?AF_UNIX:AF_INET), SOCK_DGRAM, 0);
    if (io->io_fd < 0) {
        return -1;
    }

    setsockopt (io->io_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (int));
    __set_nonblock (io->io_fd);
    if (bind (io->io_fd, &addrinfo, sizeof (struct sockaddr)) < 0) {
        close (io->io_fd);
        return -1;
    }
    return 0;
}

static double udpReadFromIO (struct _Net *net, NetIO_t *io, NetBuffer_t *buffer)
{
    struct sockaddr addr;
    socklen_t       sklen = sizeof (addr);

    buffer->buffer_length = __recvfrom (io->io_fd, 
                                        buffer->buffer, 
                                        buffer->buffer_size,
                                        &addr,
                                        &sklen);
    if (buffer->buffer_length > 0) {
        __addr2net (&addr, &io->io_net);
    }

    return buffer->buffer_length;
}

double udpWriteToIO (struct _Net *net, const NetIO_t *io, NetBuffer_t *buffer)
{
    struct sockaddr addr;

    buffer->buffer_length = __sendto (io->io_fd, 
                                      buffer->buffer, 
                                      buffer->buffer_length,
                                      &addr,
                                      sizeof (addr));
    return buffer->buffer_length;
}

static void udpDestroyIO (struct _Net *net, NetIO_t *io)
{
    close (io->io_fd);
}
