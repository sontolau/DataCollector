#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include "netsrv.h"
#include <ev.h>

#define __SIG__   (10)
#define __SIG_REQUEST (__SIG__ + 1)
#define __SIG_REPLY   (__SIG__ + 2)

#define WakeUpQueueProc(serv, sig)   do{ DC_signal_send (serv->sig_handle, sig);}while (0)
#define WakeUpRequestProc(serv) WakeUpQueueProc(serv, __SIG_REQUEST)
#define WakeUpReplyProc(serv)    WakeUpQueueProc(serv, __SIG_REPLY)

extern NetBuffer_t *NetBufferAlloc (Net_t *srv);
extern void NetBufferFree (Net_t *srv, NetBuffer_t *buf);
extern NetIO_t     *NetIOAlloc (Net_t *srv);
extern void NetIOFree (Net_t *srv, NetIO_t *io);
extern void NetCloseIO (Net_t *srv, NetIO_t *io);

#include "nettcp.c"
#include "netudp.c"

static NetIOHandler_t NetProtocolHandler[] = {
    [NET_TCP] = {
        .netCreateIO          = tcpCreateIO,
        .netAcceptRemoteIO    = tcpAcceptRemoteIO,
        .netReadFromIO        = tcpReadFromIO,
        .netWriteToIO         = tcpWriteToIO,
        .netDestroyIO         = tcpDestroyIO,
    },
    [NET_UDP] = {
        .netCreateIO          = udpCreateIO,
        .netAcceptRemoteIO    = NULL,
        .netReadFromIO        = udpReadFromIO,
        .netWriteToIO         = udpWriteToIO,
        .netDestroyIO         = udpDestroyIO,
    }
};

int NetIOInit (NetIO_t *io, const NetInfo_t *info)
{
    memset (io, '\0', sizeof (NetIO_t));
    io->__handler = &NetProtocolHandler[info->net_type];
    io->io_net    = *info;

    return 0;
    //return NetIOCreate (io, info);
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

NetBuffer_t *NetAllocIOBuffer (Net_t *srv, NetIO_t *io)
{
    NetBuffer_t *buf = NULL;

    buf = NetBufferAlloc (srv);
    if (buf) {
        buf->__link.next = NULL;
        buf->__link.prev = NULL;
 
        buf->io          = *io;
        DC_link_add (io->__buf_link.prev, &buf->__link);
    }

    return buf;
}

void NetFreeIOBuffer (Net_t *srv, NetBuffer_t *buf)
{
    if (buf) {
        DC_link_remove (&buf->__link);
        NetBufferFree (srv, buf);
    }
}

void NetCommitIOBuffer (Net_t *srv, NetIO_t *io)
{
    register DC_link_t *linkptr, *tmpptr;
    NetBuffer_t        *buffer;

    linkptr = io->__buf_link.next;
    while (linkptr && linkptr != &io->__buf_link) {
        tmpptr = linkptr->next;
        DC_link_remove (linkptr);

        buffer = DC_link_container_of (linkptr, NetBuffer_t, __link);
        PutNetBufferIntoReplyQueue (srv, buffer);
        linkptr = tmpptr;
    }

    WakeUpReplyProc (srv);
}

DC_INLINE void NetSetStatus (Net_t *serv, int status)
{
    if (serv->delegate && serv->delegate->willChangeStatus) {
        serv->delegate->willChangeStatus (serv, status);
    }

    serv->status = status;
}

static inline void NetProcessRequest (Net_t *serv)
{
    NetBuffer_t  *req_buf = NULL;

    while ((req_buf = GetNetBufferFromRequestQueue (serv))) { 
        //TODO: add code here to process request from remote. 
        if (serv->delegate && serv->delegate->processRequest) {
            if (serv->delegate->processRequest (serv, req_buf)) {
                PutNetBufferIntoReplyQueue (serv, req_buf);
		WakeUpReplyProc (serv);
            } else {
                NetBufferFree (serv, req_buf);
            }
        }
    }
}

static inline void NetProcessReply (Net_t *serv)
{
    NetBuffer_t *buf;

    while ((buf = GetNetBufferFromReplyQueue (serv))) {
        if (buf->io.__handler->netWriteToIO (&buf->io, buf->buffer, buf->buffer_length) <= 0) {
            buf->io.__handler->netDestroyIO (&buf->io);
            ev_io_stop (serv->ev_loop, &buf->ev_io->io_ev);
            if (buf->io.__handler->netAcceptRemoteIO) {
                NetIOFree (serv, buf->ev_io);
            }
        }
        NetBufferFree (serv, buf);
    }
}

static void NetSignalHandler (DC_sig_t sig, void *data)
{
    Net_t *serv = (Net_t*)data;

    switch (sig) {
        case __SIG_REQUEST:
        {
            NetProcessRequest (serv);
        }
        break;
        case __SIG_REPLY:
        {
            NetProcessReply (serv);
        }
        default:
            break;
    }
}

DC_INLINE void NetIOReadCallBack (struct ev_loop *ev, ev_io *w, int revents)
{
    Net_t *srv = (Net_t*)ev_userdata (ev);
    NetIO_t *io = (NetIO_t*)w->data;
    NetBuffer_t *buffer = NULL;

    buffer = NetBufferAlloc (srv);
    if (buffer == NULL) {
        //TODO: process exception.
        return;
    } else {
        buffer->buffer_size   = srv->config->buffer_size;
        buffer->buffer_length = 0;
        buffer->io            = *io;
        buffer->ev_io         = io;

        if ((buffer->buffer_length = io->__handler->netReadFromIO (&buffer->io, buffer->buffer, buffer->buffer_size)) <= 0) {
            NetBufferFree (srv, buffer);
            io->__handler->netDestroyIO (io);
            ev_io_stop (srv->ev_loop, w);
            if (io->__handler->netAcceptRemoteIO) {
                NetIOFree (srv, io);
            }
            //TODO: process exception.
            return;
        } else {
            PutNetBufferIntoRequestQueue (srv, buffer);
            WakeUpRequestProc (srv);
        }
    }
}

DC_INLINE void NetIOAcceptCallBack (struct ev_loop *ev, ev_io *w, int revents)
{
    NetIO_t *io   = (NetIO_t*)w->data;
    Net_t *srv    = (Net_t*)ev_userdata (ev);

    NetIO_t *newio = NetIOAlloc (srv);
    if (newio == NULL) {
        close (accept (io->io_fd, NULL, NULL));
        return;
    } else {
        memset (newio, '\0', sizeof (NetIO_t));
        if (io->__handler->netAcceptRemoteIO (newio, io) < 0) {
            //TODO: process exception.
            io->__handler->netDestroyIO (io);
            NetIOFree (srv, newio);
            return;
        } else {
            *newio = *io;
            ev_io_init (&newio->io_ev, NetIOReadCallBack, newio->io_fd, EV_READ);
            ev_io_start (srv->ev_loop, &newio->io_ev);
        }
    }
}

DC_INLINE int CreateSocketIO (Net_t *serv)
{
    int i;
    NetIO_t *io;

    serv->net_io = (NetIO_t*)calloc (serv->config->num_net_io, sizeof (NetIO_t));
    if (serv->net_io == NULL) {
        return -1;
    }

    for (i=0; i<serv->config->num_net_io &&
        serv->delegate &&
        serv->delegate->getNetInfoWithIndex; i++) {
        io = &serv->net_io[i];

        serv->delegate->getNetInfoWithIndex (serv, &io->io_net, i);
        io->__handler = &NetProtocolHandler[io->io_net.net_type];
        if (io->__handler->netCreateIO (io, &io->io_net, serv->config) < 0) {
            return -1;
        } else {
            if (io->__handler->netAcceptRemoteIO) {
                ev_io_init (&io->io_ev, NetIOAcceptCallBack, io->io_fd, EV_READ);
            } else {
                ev_io_init (&io->io_ev, NetIOReadCallBack, io->io_fd, EV_READ);
            }
            ev_io_start (serv->ev_loop, &io->io_ev);
            io->io_ev.data = io;
        }
    }

    return 0;
}

DC_INLINE void DestroySocketIO (Net_t *serv)
{
    int i;

    for (i=0; i<serv->config->num_net_io &&
                serv->net_io; i++) {
        serv->net_io[i].__handler->netDestroyIO (&serv->net_io[i]);
        ev_io_stop (serv->ev_loop, &serv->net_io[i].io_ev);
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
<<<<<<< HEAD
    if (DC_signal_wait_asyn (serv->sig_handle, 1000, NetSignalHandler, serv) < 0
        || DC_signal_wait_asyn (serv->sig_handle, 1000, NetSignalHandler, serv) < 0) {
=======
    if (DC_signal_wait_asyn (serv->sig_handle, 1000, NetSignalHandler, serv) < 0 ||
        DC_signal_wait_asyn (serv->sig_handle, 1000, NetSignalHandler, serv) < 0) {
>>>>>>> bbacc31516e357390ec726bc0f09b735ebaa8b94
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

#include <signal.h>

DC_INLINE int RunNet (Net_t *serv)
{
    ev_timer  timer;
    ev_signal sig_handler;

    if (CreateSocketIO (serv) < 0) {
        fprintf (stderr, "CreateSocketIO failed due to %s\n", strerror (errno));
        return -1;
    }

    timer.data = serv;
    ev_timer_init (&timer, TimerCallBack, serv->config->timer_interval, 1);
    ev_timer_start (serv->ev_loop, &timer);

    sig_handler.data = serv;
    ev_signal_init (&sig_handler, SignalCallBack, SIGINT);
    ev_signal_start(serv->ev_loop, &sig_handler);

    if (serv->delegate && serv->delegate->willRunNet) {
        serv->delegate->willRunNet (serv);
    }

    ev_loop (serv->ev_loop, 0);

    if (serv->delegate && serv->delegate->willStopNet) {
        serv->delegate->willStopNet (serv);
    }

    ev_timer_stop (serv->ev_loop, &timer);
    ev_signal_stop (serv->ev_loop, &sig_handler);

    DestroySocketIO (serv);

    return 0;
}

DC_INLINE void ReleaseNet (Net_t *serv)
{
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
    pid_t pid;

    serv->config   = config;
    serv->delegate = delegate;

    if (config->chdir && chdir (config->chdir) < 0) {
        fprintf (stderr, "chdir call failed.\n");
        return -1;
    }

    if (config->daemon) {
        pid = fork ();

        if (pid > 0) {
            exit (EXIT_SUCCESS);
        } else if (pid < 0) {
            fprintf (stderr, "fork call failed due to %s\n", strerror (errno));
            return -1;
        }
    }

    if (!(ret = InitNet (serv))) {
        ret = RunNet (serv);
    }

    ReleaseNet (serv);

    return ret;
}

void NetStop (Net_t *serv)
{
    kill (0, SIGINT);
}
