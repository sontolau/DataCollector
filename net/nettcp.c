#include "netutil.c"

static int tcpCreateIO (NetIO_t *io)
{
    int flag = 1;
    NetAddr_t *info = io->addr_info;

    io->fd = socket (io->local_addr.ss.ss_family , SOCK_STREAM, 0);
    if (io->fd < 0) {
        return -1;
    }

    if (info->net_flag & NET_F_BIND) {
        setsockopt (io->fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (int));
        if (bind (io->fd, (struct sockaddr*)&io->local_addr.ss, io->local_addr.sock_length) < 0 ||
            listen (io->fd, 1000) < 0) {
            close (io->fd);
            return -1;
        }
    } else {
        if (connect (io->fd, (struct sockaddr*)&io->local_addr.ss, io->local_addr.sock_length) < 0) {
            close (io->fd);
            return -1;
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
    newio->fd = accept (io->fd, (struct sockaddr*)&newio->local_addr.ss, &newio->local_addr.sock_length);
    if (newio->fd < 0) {
        return -1;
    }

    return 0;
}

static long tcpReadFromIO (NetIO_t *io, unsigned char *buf, unsigned int szbuf)
{
    return __recv (io->fd, buf, szbuf);
}

static long tcpWriteToIO (const NetIO_t *io, const unsigned char *bytes, unsigned int len)
{
    return __send (io->fd, bytes, len);
}

static void tcpCloseIO (NetIO_t *io)
{
    close (io->fd);
}
