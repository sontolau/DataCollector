#include "netutil.c"

static int tcpCreateIO (NetIO_t *io)
{
    int flag = 1;
    NetAddr_t *info = io->addr_info;

    io->fd = socket (io->ss.ss_family , SOCK_STREAM, 0);
    if (io->fd < 0) {
        return -1;
    }

    if (info->net_flag & NET_F_BIND) {
        setsockopt (io->fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (int));
        if (bind (io->fd, (struct sockaddr*)&io->ss, io->sock_len) < 0 ||
            listen (io->fd, 1000) < 0) {
            close (io->fd);
            return -1;
        }
    } else {
        if (connect (io->fd, (struct sockaddr*)&io->ss, io->sock_len) < 0) {
            close (io->fd);
            return -1;
        }
    }
    return 0;
}

static int tcpWillAcceptRemoteIO (NetIO_t *io)
{
    return 1;
}

static int tcpAcceptRemoteIO (NetIO_t *newio, const NetIO_t *io)
{
    newio->fd = accept (io->fd, (struct sockaddr*)&newio->ss, &newio->sock_len);
    if (newio->fd < 0) {
        return -1;
    }

    return 0;
}

static double tcpReadFromIO (NetIO_t *io, unsigned char *bytes, unsigned int szbuf)
{
    return __recv (io->fd, bytes, szbuf);
}

static double tcpWriteToIO (const NetIO_t *io, const unsigned char *bytes, unsigned int length)
{
    return __send (io->fd, bytes, length);
}

static void tcpDestroyIO (NetIO_t *io)
{
    close (io->fd);
}
