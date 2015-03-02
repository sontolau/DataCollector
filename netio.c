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

#include "netio.h"

extern NetBuffer_t *NetBufferAlloc (Net_t *srv);
extern void NetBufferFree (Net_t *srv, NetBuffer_t *buf);
extern NetIO_t     *NetIOAlloc (Net_t *srv);
extern void NetIOFree (Net_t *srv, NetIO_t *io);
extern void NetCloseIO (Net_t *srv, NetIO_t *io);


#include "net/nettcp.c"
#include "net/netudp.c"
#include "net/netcomm.c"

static NetIOHandler_t NetProtocolHandler[] = {
    [NET_TCP] = {
        .netCreateIO          = tcpCreateIO,
        .willAcceptRemoteIO   = tcpWillAcceptRemoteIO,
        .netAcceptRemoteIO    = tcpAcceptRemoteIO,
        .netReadFromIO        = tcpReadFromIO,
        .netWriteToIO         = tcpWriteToIO,
        .netDestroyIO         = tcpDestroyIO,
    },
    [NET_UDP] = {
        .netCreateIO          = udpCreateIO,
        .willAcceptRemoteIO   = udpWillAcceptRemoteIO,
        .netAcceptRemoteIO    = udpAcceptRemoteIO,
        .netReadFromIO        = udpReadFromIO,
        .netWriteToIO         = udpWriteToIO,
        .netDestroyIO         = udpDestroyIO,
    }
};

int NetIOInit (NetIO_t *io, const NetAddr_t *info)
{
    io->__handler = &NetProtocolHandler[info->net_type];
    __net2addr (info, (struct sockaddr*)&io->ss, &io->sock_len);
    io->addr_info = (NetAddr_t*)info;

    return 0;
}

DC_INLINE int PutNetBufferIntoQueue (Net_t *serv, NetBuffer_t *buf, DC_queue_t *queue)
{
    int ret;
    DC_mutex_lock (&serv->serv_lock, LOCK_IN_WRITE, 1);
    ret = DC_queue_push (queue, (qobject_t)buf, 1);
    DC_mutex_unlock (&serv->serv_lock);
    return ret;
}
#define PutNetBufferIntoRequestQueue(serv, buf)\
    PutNetBufferIntoQueue(serv, buf, ((DC_queue_t*)&serv->request_queue))

#define PutNetBufferIntoReplyQueue(serv, buf) \
    PutNetBufferIntoQueue(serv, buf, ((DC_queue_t*)&serv->reply_queue))


DC_INLINE NetBuffer_t *GetNetBufferFromQueue (Net_t *serv, DC_queue_t *queue)
{
    NetBuffer_t *nbuf;

    DC_mutex_lock (&serv->serv_lock, LOCK_IN_WRITE, 1);
    nbuf = (NetBuffer_t*)DC_queue_pop (queue);
    DC_mutex_unlock (&serv->serv_lock);

    return nbuf;
}

DC_INLINE int NetBufferHasNext (Net_t *serv, DC_queue_t *queue)
{
    int ret = 0;

    DC_mutex_lock (&serv->serv_lock, LOCK_IN_READ, 1);
    ret = DC_queue_is_empty (queue);
    DC_mutex_unlock (&serv->serv_lock);

    return (!ret);
}

#define GetNetBufferFromRequestQueue(serv) \
    GetNetBufferFromQueue(serv, ((DC_queue_t*)&serv->request_queue))

#define GetNetBufferFromReplyQueue(serv) \
    GetNetBufferFromQueue(serv, ((DC_queue_t*)&serv->reply_queue))

#define HasMoreBufferToBeProcessed(serv) \
    NetBufferHasNext(serv, ((DC_queue_t*)&serv->request_queue))

#define HasMoreBufferToBeSent(serv) \
    NetBufferHasNext (serv, ((DC_queue_t*)&serv->reply_queue))

NetBuffer_t *NetAllocIOBuffer (Net_t *srv, NetIO_t *io)
{
    NetBuffer_t *buf = NULL;

    buf = NetBufferAlloc (srv);
    if (buf) {
        buf->PRI (link).next = NULL;
        buf->PRI (link).prev = NULL;
 
        buf->io          = NetIOGet (io);
        DC_link_add (io->PRI (buf_link).prev, &buf->PRI (link));
    }

    return buf;
}

void NetFreeIOBuffer (Net_t *srv, NetBuffer_t *buf)
{
    if (buf) {
        DC_link_remove (&buf->PRI (link));
        NetIOFree (srv, buf->io);
        NetBufferFree (srv, buf);
    }
}

static void NetProcessReply (DC_thread_t *thread, void *data);
void NetCommitIOBuffer (Net_t *srv, NetIO_t *io)
{
    register DC_link_t *linkptr, *tmpptr;
    NetBuffer_t        *buffer;

    NetIOLock (io);
    linkptr = io->PRI (buf_link).next;
    while (linkptr && linkptr != &io->PRI (buf_link)) {
        tmpptr = linkptr->next;
        DC_link_remove (linkptr);

        buffer = DC_link_container_of (linkptr, NetBuffer_t, PRI (link));
        PutNetBufferIntoReplyQueue (srv, buffer);
        linkptr = tmpptr;
    }
    NetIOUnlock (io);

    if (DC_thread_get_status (((DC_thread_t*)&srv->reply_thread)) == THREAD_STAT_IDLE) {
        DC_thread_run (&srv->reply_thread, NetProcessReply, srv);
    }
}

/*
DC_INLINE void NetSetStatus (Net_t *serv, int status)
{
    if (serv->delegate && serv->delegate->willChangeStatus) {
        serv->delegate->willChangeStatus (serv, status);
    }

    serv->status = status;
}
*/
static void ProcessRequestCore (DC_task_t *task ,void *data)
{
    Net_t *serv = (Net_t*)data;
    NetBuffer_t *buf = NULL;
    //int ret = 0;

    printf ("[libdc] Process REQUEST queue ... ... \n");
    while ((buf = GetNetBufferFromRequestQueue (serv))) {
        //TODO: add code here to process request from remote.
        if (serv->delegate && serv->delegate->processRequest) {
            serv->delegate->processRequest (serv, buf);
        }

        NetIOFree (serv, buf->io);
        NetBufferFree (serv, buf);
/*
        if (ret) {
            PutNetBufferIntoReplyQueue (serv, buf);
            if (DC_thread_get_status (((DC_thread_t*)&serv->reply_thread)) == THREAD_STAT_IDLE) {
                DC_thread_run (&serv->reply_thread, NetProcessReply, serv);
            }
        } else {
            NetBufferFree (serv, buf);
        }
*/
    }
}

static void NetProcManager (DC_thread_t *thread,
                            void *data)
{
    Net_t *serv = (Net_t*)data;
    int          i = 0;

    printf ("[libdc] Running PROCESS task .....\n");
    for (i=0; i<serv->config->num_process_threads && HasMoreBufferToBeProcessed (serv); i++) {
        DC_thread_pool_manager_run_task (&serv->core_proc_pool, 
                                         ProcessRequestCore, 
                                         NULL, 
                                         NULL, 
                                         serv, 1);
    }
}

static void NetProcessReply (DC_thread_t *thread, void *data)
{
    Net_t *serv = (Net_t*)data;
    NetBuffer_t *buf = NULL;;

    printf ("[libdc] Running REPLY task ... ...\n");
    while ((buf = GetNetBufferFromReplyQueue (serv))) {
        if (NetIOWriteTo (buf->io, buf->buffer, buf->buffer_length) <= 0) {
            if (serv->delegate && serv->delegate->willCloseNetIO) {
                serv->delegate->willCloseNetIO (serv, buf->io);
            }
            NetIODestroy (buf->io);

//            ev_io_stop (serv->ev_loop, &buf->ev_io->ev);
//            if (buf->io.__handler->willAcceptRemoteIO) {
//                NetIOFree (serv, buf->ev_io);
//            }
        }
        NetIOFree (serv, buf->io);
        NetBufferFree (serv, buf);
    }
}

DC_INLINE void NetIOReadCallBack (struct ev_loop *ev, ev_io *w, int revents)
{
    Net_t *srv = (Net_t*)ev_userdata (ev);
    NetIO_t *io = (NetIO_t*)w->data;
    NetIO_t *newio = NULL;
    NetBuffer_t *buffer = NULL;
  
    if (!(buffer = NetBufferAlloc (srv))) {
        //TODO: process exception.
EXCEPT:
        if (buffer) {
            if (buffer->io) NetIOFree (srv, buffer->io);
            NetBufferFree (srv, buffer);
        }

        return;
    }

    if (!NetIOWillAcceptRemote (io)) {
        if ((newio = NetIOAlloc (srv))) {
            NetIOInit (newio, io->addr_info);
            newio->fd = io->fd;
            newio->local = io;
            io        = newio;
        } else {
            //TODO: process exception.
            goto EXCEPT;
        }
    } else {
        io = NetIOGet (io);
    }

    buffer->buffer_size   = srv->config->buffer_size;
    buffer->buffer_length = 0;
    buffer->io            = io;
    if ((buffer->buffer_length = NetIOReadFrom  (io, 
                                                 buffer->buffer, 
                                                 buffer->buffer_size)) <= 0) {
        if (srv->delegate && srv->delegate->willCloseNetIO) {
            srv->delegate->willCloseNetIO (srv, io);
        }

        NetIODestroy (io);
        ev_io_stop (srv->ev_loop, w);
        goto EXCEPT;
    } else {
        PutNetBufferIntoRequestQueue (srv, buffer);
        if (DC_thread_get_status (((DC_thread_t*)&srv->manager_thread)) == THREAD_STAT_IDLE) {
            DC_thread_run (&srv->manager_thread, NetProcManager, srv);
        }
    }
}

DC_INLINE void NetIOAcceptCallBack (struct ev_loop *ev, 
                                    ev_io *w, 
                                    int revents)
{
    NetIO_t *io   = (NetIO_t*)w->data;
    Net_t *srv    = (Net_t*)ev_userdata (ev);
    //NetIO_t   tmpio;

    NetIO_t *newio = NetIOAlloc (srv);
    if (newio == NULL) {
        //TODO: process exception.
        //io->__handler->netAcceptRemoteIO (&tmpio, io);
        //io->__handler->netDestroyIO (&tmpio);

        return;
    } else {
        NetIOInit (newio, io->addr_info);
        
        if (NetIOAcceptRemote (io, newio) < 0) {
        //if (io->__handler->netAcceptRemoteIO (newio, io) < 0) {
            //TODO: process exception.
            if (srv->delegate && srv->delegate->willCloseNetIO) {
                srv->delegate->willCloseNetIO (srv, io);
            }
            NetIODestroy (io);
            NetIOFree (srv, newio);
            return;
        } else {
            if (srv->delegate && srv->delegate->willAcceptRemoteNetIO ) {
                if (!srv->delegate->willAcceptRemoteNetIO (srv, newio)) {
                    NetIODestroy (newio);
                    NetIOFree (srv, newio);
                    return;
                }
            }
            newio->local   = io;
            newio->ev.data = newio;
            ev_io_init (&newio->ev, 
                        NetIOReadCallBack, 
                        newio->fd, EV_READ);
            ev_io_start (srv->ev_loop, &newio->ev);
        }
    }
}

DC_INLINE int CreateSocketIO (Net_t *serv)
{
    int i;
    NetIO_t *io;
    //NetAddr_t  ioinfo;

    serv->net_addr_array = (NetAddr_t*)calloc (serv->config->num_listeners, sizeof (NetAddr_t));
    if (serv->net_addr_array == NULL) {
        return -1;
    }

    serv->net_io = (NetIO_t*)calloc (serv->config->num_listeners, sizeof (NetIO_t));
    if (serv->net_io == NULL) {
        return -1;
    }

    for (i=0; i<serv->config->num_listeners &&
        serv->delegate &&
        serv->delegate->getNetListenerAddressWithIndex; i++) {
        io = &serv->net_io[i];

        serv->delegate->getNetListenerAddressWithIndex (serv, &serv->net_addr_array[i], i);
        NetIOInit (io, &serv->net_addr_array[i]);

        //io->__handler = &NetProtocolHandler[io->addr_info.net_type];
        if (NetIOCreate (io) < 0) {
            return -1;
        } else {
            if (NetIOWillAcceptRemote (io)) {
                ev_io_init (&io->ev, NetIOAcceptCallBack, io->fd, EV_READ);
            } else {
                ev_io_init (&io->ev, NetIOReadCallBack, io->fd, EV_READ);
            }
            ev_io_start (serv->ev_loop, &io->ev);
            io->ev.data = io;
        }
    }

    return 0;
}

DC_INLINE void DestroySocketIO (Net_t *serv)
{
    int i;

    for (i=0; i<serv->config->num_listeners &&
                serv->net_io; i++) {
        NetIODestroy (((NetIO_t*)&serv->net_io[i]));
        ev_io_stop (serv->ev_loop, &serv->net_io[i].ev);
    }

    if (serv->net_io) free (serv->net_io);
    if (serv->net_addr_array) free (serv->net_addr_array);
}


DC_INLINE int InitNet (Net_t *serv)
{
    NetConfig_t *config     = serv->config;
    NetDelegate_t *delegate = serv->delegate;

    if (delegate && delegate->willInitNet && delegate->willInitNet (serv) < 0) {
        return -1;
    }

    if (config->max_buffers && DC_buffer_pool_init (&serv->net_buffer_pool, 
                             config->max_buffers,
                             config->buffer_size+sizeof (NetBuffer_t)) < 0) {
        return -1;
    }

    if (config->max_requests && DC_buffer_pool_init (&serv->net_io_pool,
                             config->max_requests,
                             sizeof (NetIO_t)) < 0) {
        return -1;
    }

    if (DC_mutex_init (&serv->buf_lock, 0, NULL, NULL) < 0) {
        return -1;
    }

    if (DC_queue_init (&serv->request_queue,
                       config->queue_size,
                       0) < 0) {
        return -1;
    }
        
    if (DC_queue_init (&serv->reply_queue,
                       config->queue_size,
                       0) < 0) {
        return -1;
    }

    if (DC_mutex_init (&serv->serv_lock, 1, NULL, NULL)) {
        return -1;
    }

    if (DC_thread_init (&serv->manager_thread, NULL, 0, NULL)) {
        return -1;
    }

    if (DC_thread_init (&serv->reply_thread, NULL, 0, NULL)) {
        return -1;
    } 

    if (DC_thread_pool_manager_init (&serv->core_proc_pool, 
                                     serv->config->num_process_threads?
                                     serv->config->num_process_threads:1, 
                                     NULL)) {
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

#define DP(x) fprintf(stderr, x)
DC_INLINE void ReleaseNet (Net_t *serv)
{
    ev_loop_destroy (serv->ev_loop);
    DC_queue_destroy (&serv->request_queue);
    DC_queue_destroy (&serv->reply_queue);
    DC_buffer_pool_destroy (&serv->net_io_pool);
    DC_buffer_pool_destroy (&serv->net_buffer_pool);
    DC_mutex_destroy (&serv->buf_lock);
    DC_mutex_destroy (&serv->serv_lock);

    DC_thread_destroy (&serv->reply_thread);
    DC_thread_destroy (&serv->manager_thread);
    DC_thread_pool_manager_destroy (&serv->core_proc_pool);
}

int NetRun (Net_t *serv, NetConfig_t *config, NetDelegate_t *delegate)
{
    int ret = 0;
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

    if (!InitNet (serv)) {
        ret = RunNet (serv);
        ReleaseNet (serv);
    }

    return ret;
}

void NetStop (Net_t *serv)
{
    kill (0, SIGINT);
}
