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

#include "net/nettcp.c"
#include "net/netudp.c"
#include "net/netcomm.c"

NetIOHandler_t NetProtocolHandler[] = {
    [NET_TCP] = {
        .netCreateIO          = tcpCreateIO,
        .willAcceptRemoteIO   = tcpWillAcceptRemoteIO,
        .netAcceptRemoteIO    = tcpAcceptRemoteIO,
        .netReadFromIO        = tcpReadFromIO,
        .netWriteToIO         = tcpWriteToIO,
        .netCloseIO         = tcpCloseIO,
    },
    [NET_UDP] = {
        .netCreateIO          = udpCreateIO,
        .willAcceptRemoteIO   = udpWillAcceptRemoteIO,
        .netAcceptRemoteIO    = udpAcceptRemoteIO,
        .netReadFromIO        = udpReadFromIO,
        .netWriteToIO         = udpWriteToIO,
        .netCloseIO         = udpCloseIO,
    }
};

int NetIOInit (NetIO_t *io,
               const NetAddr_t *info,
               void (*release)(NetIO_t*,void*),
               void *data)
{
    if (info) {
        io->__handler = &NetProtocolHandler[info->net_type];
        __net2addr (info, &io->local_addr);
        io->addr_info = (NetAddr_t*)info;
    }

    io->refcount   = 1;
    io->release_cb = release;
    io->cb_data    = data;

    DC_mutex_init (&io->io_lock, 0, NULL, NULL);
    DC_link_init2 (io->PRI (conn_link), NULL);
    DC_link_init2 (io->PRI (buffer_link), NULL);
    return 0;
}


DC_INLINE int PutObjectIntoQueue (Net_t *serv, void *obj, DC_queue_t *queue)
{
    int ret;
    NetLockContext (serv);
    ret = DC_queue_push (queue, (qobject_t)obj, 1);
    NetUnlockContext (serv);
    return ret;
}

DC_INLINE void *GetObjectFromQueue (Net_t *serv, DC_queue_t *queue)
{
    void *obj = NULL;
    NetLockContext (serv);
    obj = (void*)DC_queue_pop (queue);
    NetUnlockContext (serv);
    return obj;
}

DC_INLINE int NetIOHasNext (Net_t *serv, DC_queue_t *queue)
{
    int ret = 0;

    NetLockContext (serv);
    ret = DC_queue_is_empty (queue);
    NetUnlockContext (serv);

    return (!ret);
}

#define PutBufferIntoRequestQueue(serv, buf)\
    PutObjectIntoQueue(serv, buf, ((DC_queue_t*)&serv->request_queue))

#define PutIOIntoReplyQueue(serv, io) \
    PutObjectIntoQueue(serv, (void*)io, ((DC_queue_t*)&serv->reply_queue))

#define GetBufferFromRequestQueue(serv) \
    (NetBuffer_t*)GetObjectFromQueue(serv, ((DC_queue_t*)&serv->request_queue))

#define GetIOFromReplyQueue(serv) \
    (NetIO_t*)GetObjectFromQueue(serv, ((DC_queue_t*)&serv->reply_queue))

#define HasMoreBufferToBeProcessed(serv) \
    NetIOHasNext(serv, ((DC_queue_t*)&serv->request_queue))

#define HasMoreIOToBeSent(serv) \
    NetIOHasNext(serv, ((DC_queue_t*)&serv->reply_queue))



static void ProcessRequestCore (DC_task_t *task ,void *data)
{
    Net_t *serv = (Net_t*)data;
    NetBuffer_t *buf = NULL;

    while ((buf = GetBufferFromRequestQueue (serv))) {
        //TODO: add code here to process request from remote.
        if (serv->delegate && serv->delegate->processBuffer) {
            serv->delegate->processBuffer (serv, buf);
        }
        NetIORelease (buf->io);
        NetFreeBuffer (serv, buf);
    }
}

static void NetProcManager (DC_thread_t *thread,
                            void *data)
{
    Net_t *serv = (Net_t*)data;
    int          i = 0;

    for (i=0; i<serv->config->num_process_threads && 
                HasMoreBufferToBeProcessed (serv); i++) {
        DC_thread_pool_manager_run_task (&serv->core_proc_pool, 
                                         ProcessRequestCore, 
                                         NULL, 
                                         NULL, 
                                         serv, 
                                         1);
    }
}

static void NetProcessReply (DC_thread_t *thread, void *data)
{
    Net_t *serv = (Net_t*)data;
    NetBuffer_t *buf = NULL;;
    NetIO_t     *io  = NULL;
    DC_link_t   *linkptr = NULL,
                *nextlink= NULL;
    DC_link_t   tmplink;
    int         iostatus = 1;

    // fetch each outgoing IO from queue.
    while ((io = GetIOFromReplyQueue (serv))) {
        iostatus = 1;
        // set buffer link from IO to local.
        DC_link_assign (&io->PRI (buffer_link), &tmplink);
        DC_link_init2 (io->PRI (buffer_link), NULL);
        NetIOUnlock (io);

        // loop link to get all buffers to send.
        linkptr = tmplink.next;
        while (linkptr && linkptr != &tmplink) {
            nextlink = linkptr->next;
            buf = DC_link_container_of (linkptr, NetBuffer_t, PRI (buffer_link));
            if (iostatus && (int)NetIOWriteTo (io, buf->buffer, buf->buffer_length) <= 0) {
                iostatus = 0;
                NetIORelease (io->to);
            }
            NetBufferRemoveIO (serv, buf);
            NetFreeBuffer (serv, buf);
            linkptr = nextlink;
        } 
    }
}

void NetCommitIO (Net_t *srv, NetIO_t *io)
{
    // put current io to the queue and wake up to send.
    NetIOLock (io);
    PutIOIntoReplyQueue (srv, io);
    if (DC_thread_get_status (((DC_thread_t*)&srv->reply_thread)) != THREAD_STAT_RUNNING) {
        DC_thread_run (&srv->reply_thread, NetProcessReply, srv);
    }
}

DC_INLINE void ReleaseIO (NetIO_t *io, void *data)
{
    Net_t *net = (Net_t*)data;
    
    NetFreeIO (net, io);
}

DC_INLINE void NetIOReadCallBack (struct ev_loop *ev, ev_io *w, int revents)
{
    Net_t *srv = (Net_t*)ev_userdata (ev);
    NetIO_t *io = (NetIO_t*)w->data;
    NetIO_t *newio = NULL;
    NetBuffer_t *buffer = NULL;
    
    // allocate a buffer and IO to receive data.
    if (!((buffer = NetAllocBuffer (srv)) &&
         (newio  = NetAllocIO (srv)))) {
        //TODO: process exception.
        if (buffer) NetFreeBuffer (srv, buffer);
        if (newio)  NetFreeIO (srv, io);
        return;
    } else {
        // copy meta data from local IO.
        NetIOInit (newio, io->addr_info, ReleaseIO, srv);
        newio->fd = io->fd;
        newio->to = io;
        newio->ssl= io->ssl; 
    }

    buffer->buffer_size   = srv->config->buffer_size;
    buffer->buffer_length = 0;
    buffer->io            = newio;

    // read data.
    if ((int)(buffer->buffer_length = NetIOReadFrom  (buffer->io, 
                                                      buffer->buffer,
                                                      buffer->buffer_size)) <= 0) {
        NetIORelease (buffer->io);
        NetIORelease (io);
        NetFreeBuffer (srv, buffer);
        return;
    } else {
        Dlog ("[libdc] received %u bytes.\n", buffer->buffer_length);
        // put data buffer into queue and wake up to process.
        if (NetIOConnected (io)) {
            NetIOLock (io);
            io->timer = srv->timer;
            NetIOUpdate (io);
            NetIOUnlock (io);
        }

        PutBufferIntoRequestQueue (srv, buffer);
        if (DC_thread_get_status (((DC_thread_t*)&srv->manager_thread)) == THREAD_STAT_IDLE) {
            DC_thread_run (&srv->manager_thread, NetProcManager, srv);
        }
    }
}

DC_INLINE void ReleaseConnectionIO (NetIO_t *io, void *data)
{
    Net_t *net = (Net_t*)data;

    if (NetIOConnected (io)) {
        Dlog ("[libdc] disconnect remote peer.\n");
        if (net->delegate && net->delegate->willCloseNetIO) {
            net->delegate->willCloseNetIO (net, io);
        }

        NetIODisconnect (io);
        ev_io_stop (net->ev_loop, &io->ev);
    }

    NetIOClose (io);
    NetFreeIO (net, io);
}

DC_INLINE void NetIOAcceptCallBack (struct ev_loop *ev, 
                                    ev_io *w, 
                                    int revents)
{
    NetIO_t *io   = (NetIO_t*)w->data;
    Net_t *srv    = (Net_t*)ev_userdata (ev);

    NetIO_t *newio = NetAllocIO (srv);
    if (newio == NULL) {
        //TODO: process exception.
        Dlog ("[libdc] ERROR: can not allocate memory, system is busy.\n");
        return;
    } else {
        NetIOLock (io);
        NetIOInit (newio, io->addr_info, ReleaseConnectionIO, srv);
        
        if (NetIOAcceptRemote (io, newio) < 0) {
            //TODO: process exception.
            NetIORelease (io);
            NetIORelease (newio);
            return;
        } else {
            if (srv->delegate && srv->delegate->willAcceptRemoteNetIO ) {
                if (!srv->delegate->willAcceptRemoteNetIO (srv, newio)) {
                    NetIORelease (newio);
                    return;
                }
            }

            NetIOConnect (newio, io);
            newio->timer = srv->timer;
            newio->ev.data = newio;
            ev_io_init (&newio->ev, 
                        NetIOReadCallBack, 
                        newio->fd, EV_READ);
            ev_io_start (srv->ev_loop, &newio->ev);
            NetIOUnlock (io);
        }
    }
}

DC_INLINE void ReleaseLocalIO (NetIO_t *io, void *data)
{
    NetIOClose (io);
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
        serv->delegate->getNetAddressWithIndex; i++) {
        io = &serv->net_io[i];

        serv->delegate->getNetAddressWithIndex (serv, &serv->net_addr_array[i], i);
        NetIOInit (io, &serv->net_addr_array[i], ReleaseLocalIO, serv);

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

DC_INLINE void CheckNetIOConn (DC_thread_t *thread, void *data)
{
    Net_t *net = (Net_t*)data;
    NetIO_t *local_io = NULL,
            *io= NULL;
    register DC_link_t *linkptr = NULL,
                       *nextlink= NULL;
    DC_link_t          tmplink;
    int i = 0;

    // check connections for each IO.
    for (i=0; i<net->config->num_listeners; i++) {
        local_io = &net->net_io[i];
        
        NetIOLock (local_io);
        // loop each connected client IO and to check which IO is 
        // the lastest connected IO.
        linkptr = local_io->PRI (conn_link).prev;
        while (linkptr && linkptr != &local_io->PRI (conn_link)) {
            io = DC_link_container_of (linkptr, NetIO_t, PRI (conn_link));
            if (net->timer - io->timer < net->config->conn_timeout) {
                break;
            }
            linkptr = linkptr->prev;
        }

        // if no clients are found disconnected then go to next.
        if (linkptr == &local_io->PRI (conn_link)) {
            NetIOUnlock (local_io);
            continue;
        }

        // cut off all disconnected IO from local and 
        // set them on a new link.
        tmplink.next = linkptr->next;
        tmplink.prev = local_io->PRI (conn_link).prev;

        linkptr->next->prev = &tmplink;
        local_io->PRI (conn_link).prev->next = &tmplink;

        linkptr->next = &local_io->PRI (conn_link);
        local_io->PRI (conn_link).prev= linkptr;
        NetIOUnlock (local_io);

        // close each disconnected IO.
        linkptr = tmplink.next;
        while (linkptr && linkptr != &tmplink) {
            nextlink = linkptr->next;
            io = DC_link_container_of (linkptr, NetIO_t, PRI (conn_link));
            NetIORelease (io);
            linkptr = nextlink;
        }
    }
}

DC_INLINE void DestroySocketIO (Net_t *serv)
{
    int i;

    for (i=0; i<serv->config->num_listeners &&
                serv->net_io; i++) {
        NetIOClose (((NetIO_t*)&serv->net_io[i]));
        ev_io_stop (serv->ev_loop, &serv->net_io[i].ev);
    }

    if (serv->net_io) free (serv->net_io);
    if (serv->net_addr_array) free (serv->net_addr_array);
}


DC_INLINE int InitNet (Net_t *serv)
{
    NetConfig_t *config     = serv->config;
    NetDelegate_t *delegate = serv->delegate;
    char          strtime[100] = {0};
    char          logpath[500] = {0};

    Dlog ("[libdc] STARTING ... ...\n");
    srandom (time (NULL));
    if (config->daemon && daemon (0, 1)) {
        Dlog ("[libdc] can not fork sub process due to %s.\n", ERRSTR);
        return -1;
    }

    if (config->chdir) chdir (config->chdir);

    if (config->log ) {
        __NOW ("%Y-%m-%d %T", strtime, sizeof (strtime)-1);
        snprintf (logpath, sizeof (logpath)-1, "%s-%s.log", config->log, strtime);
        if ((serv->logfp = fopen (logpath, "a+"))) {
            fclose (stderr);
            fclose (stdout);
            fclose (stdin);
            stderr = stdout = stdin = serv->logfp;
        }
    }
  
    serv->logfp = stderr;

    if (delegate && delegate->willInitNet && delegate->willInitNet (serv) < 0) {
        Dlog ("[libdc] initialized system failed.\n");
        return -1;
    }

    if ((config->max_buffers && DC_buffer_pool_init (&serv->net_buffer_pool, 
                             config->max_buffers,
                             config->buffer_size+sizeof (NetBuffer_t)) < 0) ||
        (config->max_requests && DC_buffer_pool_init (&serv->net_io_pool,
                             config->max_requests,
                             sizeof (NetIO_t)) < 0)) {
        Dlog ("[libdc] DC_buffer_pool_init failed at line:%d.\n", __LINE__);
        return -1;
    }

    if (DC_mutex_init (&serv->buf_lock, 0, NULL, NULL) < 0 ||
        DC_mutex_init (&serv->PRI(serv_lock), 0, NULL, NULL) < 0) {
        Dlog ("[libdc] DC_mutex_init failed at line:%d.\n", __LINE__);
        return -1;
    }

    if (DC_queue_init (&serv->request_queue,
                       config->queue_size,
                       0) < 0 ||
        DC_queue_init (&serv->reply_queue,
                       config->queue_size,
                       0) < 0) {
        Dlog ("[libdc] DC_queue_init failed at line:%d.\n", __LINE__);
        return -1;
    }
        
    if (DC_thread_init (&serv->manager_thread, NULL, NULL) <0 ||
        DC_thread_init (&serv->reply_thread, NULL, NULL) <0 ||
        DC_thread_init (&serv->conn_checker, NULL, NULL) < 0) {
        Dlog ("[libdc] DC_thread_init failed at line:%d.\n", __LINE__);
        return -1;
    }

    if (DC_thread_pool_manager_init (&serv->core_proc_pool, 
                                     serv->config->num_process_threads?
                                     serv->config->num_process_threads:1, 
                                     NULL)) {
        Dlog ("[libdc] DC_thread_pool_manager_init failed at line:%d.\n", __LINE__);
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
    if (DC_thread_get_status (((DC_thread_t*)&srv->conn_checker)) != THREAD_STAT_RUNNING) {
        DC_thread_run (&srv->conn_checker, CheckNetIOConn, srv);
    }

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
        Dlog ("[libdc] CreateSocketIO failed due to %s\n", ERRSTR);
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
    Dlog ("[libdc] QUITING ... ...\n");
    ev_loop_destroy (serv->ev_loop);
    DC_queue_destroy (&serv->request_queue);
    DC_queue_destroy (&serv->reply_queue);
    DC_buffer_pool_destroy (&serv->net_io_pool);
    DC_buffer_pool_destroy (&serv->net_buffer_pool);
    DC_mutex_destroy (&serv->buf_lock);
    DC_mutex_destroy (&serv->PRI(serv_lock));
    fclose (serv->logfp);
    DC_thread_destroy (&serv->reply_thread);
    DC_thread_destroy (&serv->manager_thread);
    DC_thread_pool_manager_destroy (&serv->core_proc_pool);
}

int NetRun (Net_t *serv, NetConfig_t *config, NetDelegate_t *delegate)
{
    int ret = 0;

    serv->config   = config;
    serv->delegate = delegate;

    if (!(ret = InitNet (serv))) {
        ret = RunNet (serv);
        ReleaseNet (serv);
    }

    return ret;
}

void NetStop (Net_t *serv)
{
    kill (0, SIGINT);
}
