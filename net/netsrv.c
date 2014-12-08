#include "netsrv.h"

#define __SIG__   (10)
#define __SIG_REQUEST (__SIG__ + 1)
#define __SIG_REPLY   (__SIG__ + 2)

#define WakeUpQueueProc(serv, sig)   do{ DC_signal_send (serv->sig_handle, sig);}while (0)
#define WakeUpRequestProc(serv) WakeUpQueueProc(serv, __SIG_REQUEST)
#define WakeUpReplyProc(serv)    WakeUpQueueProc(serv, __SIG_REPLY)

DC_INLINE long __recv (int sock, unsigned char *buf, unsigned int size)
{
    long szread;

    while (1) {
        szread = recv (sock, buf, size, 0);
        if (szread < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }

    return szread;
}

DC_INLINE long __send (int sock, unsigned char *buf, unsigned int size)
{
    long szwrite;

    while (1) {
        szwrite = send (sock, buf, size, 0);
        if (szwrite < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }

    return szwrite;
}

DC_INLINE long __recvfrom (int sock, unsigned char *buf, unsigned int size,
                           struct sockaddr *addr, socklen_t *sklen)
{
    long szread;

    while (1) {
        szread = recvfrom (sock, buf, size, 0, addr, sklen);
        if (szread < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }

    return szread;
}

DC_INLINE long __sendto (int sock, unsigned char *buf, unsigned int size,
                         struct sockaddr *addr, socklen_t sklen)
{
    long szwrite;

    while (1) {
        szwrite = sendto (sock, buf, size, 0, addr, sklen);
        if (szwrite < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }

    return szwrite;
}


DC_INLINE void SetNonblockFD (int fd)
{
    int flag = 0;

    if ((flag = fcntl (fd, F_GETFL)) < 0) {
        return;
    }

    flag |= O_NONBLOCK;

    fcntl (fd, F_SETFL, &flag);
}

DC_INLINE int PutNetBufferIntoQueue (Net_t *serv, NetBuffer_t *buf, DC_queue_t *queue)
{
    int ret;

    pthread_rwlock_wrlock (&serv->serv_lock);
    ret = DC_queue_push (queue, (qobject_t)buf, 1);
    pthread_rwlock_unlock (&serv->serv_lock);

    return ret;
}
#define PutNetBufferIntoRequestQueue(serv, buf)\
    PutNetBufferIntoQueue(serv, buf, ((DC_queue_t*)&serv->request_queue))

#define PutNetBufferIntoReplyQueue(serv, buf) \
    PutNetBufferIntoQueue(serv, buf, ((DC_queue_t*)&serv->reply_queue))


DC_INLINE NetBuffer_t *GetNetBufferFromQueue (Net_t *serv, DC_queue_t *queue)
{
    NetBuffer_t *nbuf;

    pthread_rwlock_wrlock (&serv->serv_lock);
    nbuf = (NetBuffer_t*)DC_queue_pop (queue);
    pthread_rwlock_unlock (&serv->serv_lock);

    return nbuf;
}
#define GetNetBufferFromRequestQueue(serv) \
    GetNetBufferFromQueue(serv, ((DC_queue_t*)&serv->request_queue))

#define GetNetBufferFromReplyQueue(serv) \
    GetNetBufferFromQueue(serv, ((DC_queue_t*)&serv->reply_queue))

DC_INLINE void CloseNetIO (Net_t *srv, NetIO_t *io)
{
    if (srv->delegate && srv->delegate->willCloseNetIO) {
        srv->delegate->willCloseNetIO (srv, io);
    }

    pthread_rwlock_wrlock (&srv->serv_lock);
    close (io->io_fd);
    ev_io_stop (srv->ev_loop, &io->io_ev);
    pthread_rwlock_unlock (&srv->serv_lock);
}

void NetCloseIO (Net_t *srv, NetIO_t *io)
{
    CloseNetIO (srv, io);
}

static void RequestQueueProc (DC_sig_t sig, void *data)
{
    NetBuffer_t  *req_buf = NULL;
    Net_t *serv = (Net_t*)data;

    switch (sig) {
        case __SIG_REQUEST:
        {
            while ((req_buf = GetNetBufferFromRequestQueue (serv))) {
                //TODO: add code here to process request from remote.
                if (serv->delegate && serv->delegate->processRequest) {
                    PutNetBufferIntoReplyQueue (serv, req_buf);
                    WakeUpReplyProc (serv);
                } else {
                    if (req_buf->io->io_net.net_type == NET_TYPE_UDP) {
                        NetIOFree (serv, req_buf->io);
                    }
                    NetBufferFree (serv, req_buf);
                }
            }
        }
            break;
        default:
            break;
    }
}

static void ReplyQueueProc (DC_sig_t sig, void *data)
{
    NetBuffer_t  *buf = NULL;
    Net_t  *serv = (Net_t*)data;
    struct sockaddr_in addr;

    switch (sig) {
        case __SIG_REPLY:
        {
            while ((buf = GetNetBufferFromReplyQueue (serv))) {
                switch (buf->io->io_net.net_type) {
                    case NET_TYPE_TCP:
                    {
                        if (__send (buf->io->io_fd, buf->buffer, buf->buffer_length) <= 0) {
                            CloseNetIO (serv, buf->io);
                            NetIOFree (serv, buf->io);
                        }
                    }
                        break;
                    case NET_TYPE_UDP:
                    {
                        addr.sin_family = AF_INET;
                        addr.sin_port   = htons(buf->io->io_net.net_port);
                        addr.sin_addr.s_addr = buf->io->io_net.net_addr;

                        if (__sendto (buf->io->io_fd, buf->buffer, buf->buffer_length, 
                                      (struct sockaddr*)&addr, sizeof (addr)) < 0) {
                            //TODO: process exception.
                        }
                        NetIOFree (serv, buf->io);
                    }
                        break;
                    default:
                        break;
                }
                NetBufferFree (serv, buf);
            }
        }
            break;
        default:
            break;
    }
}

DC_INLINE void NetIOReadCallBack (struct ev_loop *ev, ev_io *w, int revents)
{
    Net_t *srv = (Net_t*)ev_userdata (ev);
    NetIO_t *io = (NetIO_t*)w->data;
    NetIO_t *reqio = NULL;
    NetBuffer_t *buffer = NULL;
    struct sockaddr_in remote;
    socklen_t          sklen = sizeof (remote);

    buffer = NetBufferAlloc (srv);
    if (buffer == NULL) {
        //TODO: process exception.
        return;
    } else {
        buffer->buffer_size = srv->config->buffer_size;
        buffer->io          = io;
        buffer->buffer_length = 0;

        switch (io->io_net.net_type) {
            case NET_TYPE_TCP:
            {
                buffer->buffer_length = __recv (io->io_fd, buffer->buffer, buffer->buffer_size);
                if (buffer->buffer_length <= 0) {
                    //TODO: process exception.
                    NetBufferFree (srv, buffer);
                    CloseNetIO (srv, io);
                    return;
                }
            }
            case NET_TYPE_UDP:
            {
                reqio = NetIOAlloc (srv);
                if (reqio == NULL) {
                    //TODO: process exception.
                    NetBufferFree (srv, buffer);
                    return;
                } else {
                    reqio->io_fd = io->io_fd;
                    reqio->io_net.net_type = NET_TYPE_UDP;
                    buffer->io   = reqio;
                }

                buffer->buffer_length = __recvfrom (reqio->io_fd, buffer->buffer, 
                                                    buffer->buffer_size, (struct sockaddr*)&remote, &sklen);
                if (buffer->buffer_length < 0) {
                    //TODO: process exception.
                    NetIOFree (srv, reqio);
                    NetBufferFree (srv, buffer);
                    return;
                }
            }
            default:
                return;
                break;
        }

        PutNetBufferIntoRequestQueue (srv, buffer);
        WakeUpRequestProc (srv);
    }
}

DC_INLINE void NetIOAcceptCallBack (struct ev_loop *ev, ev_io *w, int revents)
{
    NetIO_t *io   = (NetIO_t*)w->data;
    Net_t *srv    = (Net_t*)ev_userdata (ev);
    struct sockaddr_in caddr;
    socklen_t sklen = sizeof (caddr);

    NetIO_t *newio = NetIOAlloc (srv);
    if (newio == NULL) {
        close (accept (io->io_fd, NULL, NULL));
        return;
    } else {
        newio->io_fd = accept (io->io_fd, (struct sockaddr*)&caddr, &sklen);
        if (newio->io_fd < 0) {
            //TODO: process exception.
            NetIOFree (srv, newio);
            return;
        } else {
            newio->io_net.net_type = io->io_net.net_type;
            newio->io_net.net_port = ntohs (caddr.sin_port);
            newio->io_net.net_addr = caddr.sin_addr.s_addr;
            ev_io_init (&newio->io_ev, NetIOReadCallBack, newio->io_fd, EV_READ);
            ev_io_start (srv->ev_loop, &newio->io_ev);
        }
    }
}

DC_INLINE int CreateSocketIO (Net_t *serv)
{
    int i;
    NetIO_t *io;
    struct sockaddr_in addrinfo;
    int flag = 1;

    serv->net_io = (NetIO_t*)calloc (serv->config->num_net_io, sizeof (NetIO_t));
    if (serv->net_io == NULL) {
        return -1;
    }

    for (i=0; i<serv->config->num_net_io &&
        serv->delegate &&
        serv->delegate->getNetInfoWithIndex; i++) {
        io = &serv->net_io[i];

        serv->delegate->getNetInfoWithIndex (serv, &io->io_net, i);

        addrinfo.sin_family = AF_INET;
        addrinfo.sin_port   = htons (io->io_net.net_port);
        addrinfo.sin_addr.s_addr = io->io_net.net_addr;

        if (io->io_net.net_type == NET_TYPE_TCP) {
            io->io_fd = socket (AF_INET, SOCK_STREAM, 0);
            if (io->io_fd < 0) {
                return -1;
            }

            setsockopt (io->io_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (int));
            SetNonblockFD (io->io_fd);
            if (bind (io->io_fd, (struct sockaddr*)&addrinfo, sizeof (addrinfo)) < 0 ||
                listen (io->io_fd, 1000) < 0) {
                return -1;
            }

            ev_io_init (&io->io_ev, NetIOAcceptCallBack, io->io_fd, EV_READ);
        } else if (io->io_net.net_type == NET_TYPE_UDP ){
            io->io_fd = socket (AF_INET, SOCK_DGRAM, 0);
            if (io->io_fd < 0) {
                return -1;
            }
            setsockopt (io->io_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (int));
            SetNonblockFD (io->io_fd);
            if (bind (io->io_fd, (struct sockaddr*)&addrinfo, sizeof (addrinfo)) < 0) {
                return -1;
            }
            ev_io_init (&io->io_ev, NetIOReadCallBack, io->io_fd, EV_READ);
        }
        ev_io_start(serv->ev_loop, &io->io_ev);
        io->io_ev.data = io;
    }

    return 0;
}

DC_INLINE void DestroySocketIO (Net_t *serv)
{
    int i;

    for (i=0; i<serv->config->num_net_io &&
                serv->net_io; i++) {
        ev_io_stop (serv->ev_loop, &serv->net_io[i].io_ev);
        close (serv->net_io[i].io_fd);
    }

    if (serv->net_io) free (serv->net_io);
}


DC_INLINE int InitNet (Net_t *serv)
{
    NetConfig_t *config     = serv->config;
    NetDelegate_t *delegate = serv->delegate;

    if (delegate && delegate->willInitNet && delegate->willInitNet (serv) < 0) {
        return -1;
    }

    if (DC_buffer_pool_init (&serv->net_buffer_pool, 
                             config->max_buffers,
                             config->buffer_size) < 0 ||
        DC_buffer_pool_init (&serv->net_io_pool,
                             config->max_peers,
                             sizeof (NetIO_t) < 0)) {
        fprintf (stderr, "DC_buffer_pool_init failed.\n");
        return -1;
    }


    if (DC_queue_init (&serv->request_queue,
                       config->queue_size,
                       0) < 0 ||
        DC_queue_init (&serv->reply_queue,
                       config->queue_size,
                       0) < 0) {
        fprintf (stderr, "DC_queue_init failed.\n");
        return -1;
    }

    if (!(serv->sig_handle = DC_signal_alloc ())) {
        fprintf (stderr, "DC_signal_alloc failed.\n");
        return -1;
    }

    if (pthread_rwlock_init (&serv->serv_lock, NULL) < 0) {
        fprintf (stderr, "pthread_rwlock_init failed.\n");
        return -1;
    }

    if (DC_signal_wait_asyn (serv->sig_handle, 1000, RequestQueueProc, serv) < 0 ||
        DC_signal_wait_asyn (serv->sig_handle, 1000, ReplyQueueProc, serv) < 0) {
        fprintf (stderr, "DC_signal_wait_asyn failed.\n");
        return -1;
    }

    serv->ev_loop = ev_loop_new (0);
    ev_set_userdata (serv->ev_loop, serv);
    return 0;
}

DC_INLINE void TimerCallBack (struct ev_loop *ev, ev_timer *w, int revents)
{
    Net_t *srv = (Net_t*)ev_userdata (ev);

    srv->timer++;
    if (srv->delegate && srv->delegate->timerout) {
        srv->delegate->timerout (srv, srv->timer);
    }
}

DC_INLINE void SignalCallBack (struct ev_loop *ev, ev_signal *w, int revents)
{
    ev_break (ev, EVBREAK_ALL);
}

DC_INLINE void RunNet (Net_t *serv)
{
    ev_timer  timer;
    ev_signal sig_handler;

    if (CreateSocketIO (serv) < 0) {
        fprintf (stderr, "CreateSocketIO failed due to %s\n", strerror (errno));
        return;
    }

    timer.data = serv;
    ev_timer_init (&timer, TimerCallBack, serv->config->timer_interval, 1);
    ev_timer_start (serv->ev_loop, &timer);

    sig_handler.data = serv;
    ev_signal_init (&sig_handler, SignalCallBack, SIGINT);
    ev_signal_start(serv->ev_loop, &sig_handler);

    
    ev_loop (serv->ev_loop, 0);

    ev_timer_stop (serv->ev_loop, &timer);
    ev_signal_stop (serv->ev_loop, &sig_handler);

    DestroySocketIO (serv);
}

DC_INLINE void ReleaseNet (Net_t *serv)
{
    if (serv->delegate && serv->delegate->willStopNet) {
        serv->delegate->willStopNet (serv);
    }

    ev_loop_destroy (serv->ev_loop);
    DC_signal_free (serv->sig_handle);
    DC_queue_destroy (&serv->request_queue);
    DC_queue_destroy (&serv->reply_queue);
    DC_buffer_pool_destroy (&serv->net_io_pool);
    DC_buffer_pool_destroy (&serv->net_buffer_pool);
    pthread_rwlock_destroy (&serv->serv_lock);
}

int NetRun (Net_t *serv, NetConfig_t *config, NetDelegate_t *delegate)
{
    int ret;
    
    memset (serv, '\0', sizeof (Net_t));
    serv->config   = config;
    serv->delegate = delegate;

    if (!(ret = InitNet (serv))) {
        RunNet (serv);
    }

    ReleaseNet (serv);

    return ret;
}

void NetStop (Net_t *serv)
{
    kill (0, SIGINT);
}
