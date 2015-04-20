/*#include <unistd.h>
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
*/
#include "libdc.h"
#include "net/nettcp.c"
#include "net/netudp.c"
#include "net/netcomm.c"


#define IOConnect(_from, _to) \
    do {\
        _from->connected = 1;\
        DC_link_add (&_to->PRI (conn_link), &_from->PRI (conn_link));\
        _from->to = _to;\
    } while (0)

#define IOConnected(_io) (_io->connected)

#define IODisconnect(_io) \
    do {\
        if (IOConnected (_io)) {\
            DC_link_remove (&_io->PRI (conn_link));\
            _io->to = NULL;\
            _io->connected = 0;\
        }\
    } while (0)

#define IOUpdate(_io) \
    do {\
        DC_link_remove (&_io->PRI (conn_link));\
        DC_link_add (&_io->PRI (conn_link), &_io->to->PRI (conn_link));\
    } while (0)



NetIOHandler_t NetProtocolHandler[] = {
    [NET_TCP] = {
        .netCreateIO          = tcpCreateIO,
        .willAcceptRemoteIO   = tcpWillAcceptRemoteIO,
        .netAcceptRemoteIO    = tcpAcceptRemoteIO,
        .netCtrlIO            = tcpCtrlIO,
        .netReadFromIO        = tcpReadFromIO,
        .netWriteToIO         = tcpWriteToIO,
        .netCloseIO         = tcpCloseIO,
    },
    [NET_UDP] = {
        .netCreateIO          = udpCreateIO,
        .willAcceptRemoteIO   = udpWillAcceptRemoteIO,
        .netAcceptRemoteIO    = udpAcceptRemoteIO,
        .netCtrlIO            = udpCtrlIO,
        .netReadFromIO        = udpReadFromIO,
        .netWriteToIO         = udpWriteToIO,
        .netCloseIO         = udpCloseIO,
    }
};

DC_INLINE void NetCloseIO (Net_t*net, NetIO_t *io);
int NetIOInit (NetIO_t *io,
               const NetAddr_t *info)
              // void (*release)(NetIO_t*,void*),
              // void *data)
{
    if (info) {
        io->__handler = &NetProtocolHandler[info->net_type];
        __net2addr (info, &io->local_addr);
        io->addr_info = (NetAddr_t*)info;
    }
/*

    io->refcount   = 1;
    io->release_cb = release;
    io->cb_data    = data;
*/
    DC_mutex_init (&io->io_lock, 0, NULL, NULL);
    DC_link_init2 (io->PRI (conn_link), NULL);
//    DC_link_init2 (io->PRI (buffer_link), NULL);
    return 0;
}

#if 0
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

#endif

static void ProcessRequestCore (DC_task_t *task ,void *data)
{
    NetBuffer_t *buf = data;
    NetProcQueue_t *queue = buf->queue;
    Net_t *net = queue->net;

    //TODO: add code here to process request from remote.
    if (net->delegate && net->delegate->processData) {
        net->delegate->processData (net, buf, queue);
    }
/*
    NetIORelease (buf->io);
*/
    NetFreeBuffer (net, buf);
}

#if 0
static void NetProcManager (DC_thread_t *thread,
                            void *data)
{
    //Net_t *serv = (Net_t*)data;
    //int          i = 0;
/*
    for (i=0; i<serv->config->num_process_threads && 
                HasMoreBufferToBeProcessed (serv); i++) {
        Dlog ("[libdc] INFO: need a task to process request[%d left].\n", serv->request_queue.length);
        DC_thread_pool_manager_run_task (&serv->core_proc_pool, 
                                         ProcessRequestCore, 
                                         NULL, 
                                         NULL, 
                                         serv, 
                                         1);
    }
*/
   
    Net_t *serv = (Net_t*)data;
    NetBuffer_t *buf = NULL;

    while ((buf = GetBufferFromRequestQueue (serv))) {
        Dlog ("[libdc] INFO: There are %d requests to be processed.\n", serv->request_queue.length);
        buf->private_data = serv;
        DC_thread_pool_manager_run_task (&serv->core_proc_pool,
                                         ProcessRequestCore,
                                         NULL,
                                         NULL,
                                         buf,
                                         1);
/*
        //TODO: add code here to process request from remote.
        if (serv->delegate && serv->delegate->processData) {
            serv->delegate->processData (serv, buf->io, buf);
        }
        NetIORelease (buf->io);
        NetFreeBuffer (serv, buf);
*/
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
            if (iostatus && (int)NetIOWriteTo (io, buf) <= 0) {
                iostatus = 0;
                NetIORelease (io->to);
            }
            if (serv->delegate && serv->delegate->didSendData) {
                serv->delegate->didSendData (serv, io, buf, iostatus);
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
#endif

DC_INLINE void NetProcCore (DC_thread_t *thread, void *data)
{

    NetProcQueue_t *queue = (NetProcQueue_t*)data;
    NetBuffer_t    *buf   = NULL;
       
    do {
        DC_mutex_lock (&queue->lock, 0, 1);
        buf = (NetBuffer_t*)DC_queue_pop (&queue->request_queue);
        DC_mutex_unlock (&queue->lock);
        if (buf) {
            buf->queue = queue;
            DC_thread_pool_manager_run_task (&queue->proc_pool,
                                             ProcessRequestCore,
                                             NULL,
                                             NULL,
                                             buf,
                                             1);
        }
    } while (buf);
}

DC_INLINE void NetReplyCore (DC_thread_t *thread, void *data)
{
    NetProcQueue_t *queue = (NetProcQueue_t*)data;
    Net_t          *net   = queue->net;
    NetBuffer_t    *buf   = NULL;
    int            status = 0;

    do {
        DC_mutex_lock (&queue->lock, 0, 1);
        buf = (NetBuffer_t*)DC_queue_pop (&queue->reply_queue);
        DC_mutex_unlock (&queue->lock);
        if (buf) {
            status = NetIOWriteTo (buf->io, buf);
            if (net->delegate && net->delegate->didSendData) {
                net->delegate->didSendData (net, buf, queue, status?0:1);
            }
            if (status <= 0) {
                NetCloseIO (net, buf->io);
            }
            NetFreeBuffer (net, buf);
        }
    } while (buf);
}

void NetPutRequestBuffer (Net_t *net, const NetBuffer_t *buf, NetProcQueue_t *queue)
{
    DC_mutex_lock (&queue->lock, 0, 1);
    DC_queue_push (&queue->request_queue, (qobject_t)buf, 1);
    queue->net = net;
    if (DC_thread_get_status (((DC_thread_t*)&queue->manager_thread)) != THREAD_STAT_RUNNING) {
        DC_thread_run (&queue->manager_thread, NetProcCore, queue);
    }
    DC_mutex_unlock (&queue->lock);
}

void NetPutReplyBuffer (Net_t *net, const NetBuffer_t *buf, NetProcQueue_t *queue)
{
    DC_mutex_lock (&queue->lock, 0, 1);
    DC_queue_push (&queue->reply_queue, (qobject_t)buf, 1);
    queue->net = net;
    if (DC_thread_get_status (((DC_thread_t*)&queue->reply_thread)) != THREAD_STAT_RUNNING) {
        DC_thread_run (&queue->reply_thread, NetReplyCore, queue);
    }
    DC_mutex_unlock (&queue->lock);
}

DC_INLINE NetProcQueue_t *GetProcQueueByHashID (Net_t *net,
                                                NetProcQueue_t *map,
                                                int num,
                                                NetBuffer_t *buf)
{
    unsigned int i;
    unsigned int hash_id = 0;
    if (net->delegate && net->delegate->getNetQueueWithHashID) {
        return &map[net->delegate->getNetQueueWithHashID(net, buf)%num];
    }

    if (buf->io->addr_info->net_flag & NET_F_IPv6) {
        for (i=0; i<sizeof (buf->buffer_addr.s6.sin6_addr); i++) {
            hash_id += ((char*)&buf->buffer_addr.s6.sin6_addr)[i];
        }
    } else {
        hash_id = buf->buffer_addr.s4.sin_addr.s_addr;
    }

    return &map[hash_id % num];
}


DC_INLINE void NetReadCallBack (struct ev_loop *ev, ev_io *w, int revents)
{
    Net_t *srv = (Net_t*)ev_userdata (ev);
    NetIO_t *io = (NetIO_t*)w->data;
    NetBuffer_t *buffer = NULL;
    NetProcQueue_t *proc_queue = NULL;

    // allocate a buffer and IO to receive data.
    if (!(buffer = NetAllocBuffer (srv))) {
        //TODO: process exception.
        return;
    } else {
        buffer->io = io;
        buffer->buffer_size = srv->config->max_sockbuf_size;
        buffer->buffer_length = 0;

        memset (buffer->buffer, '\0', buffer->buffer_size);
    }

    // read data.
    if ((int)(buffer->buffer_length = NetIOReadFrom  (buffer->io, 
                                                      buffer)) <= 0) {
        //TODO:
        NetCloseIO (srv, io);
        if (buffer->io->to) {
            NetFreeIO (srv, buffer->io);
        }
        NetFreeBuffer (srv, buffer);
        return;
    } else {
        Dlog ("[libdc] INFO: received %u bytes.\n", buffer->buffer_length);
        if (srv->delegate && srv->delegate->didReceiveData) {
            srv->delegate->didReceiveData (srv, buffer->io, buffer);
        }
        // put data buffer into queue and wake up to process.
        if (IOConnected (io)) {
            NetIOLock (io);
            io->timer = srv->timer;
            IOUpdate (io);
            NetIOUnlock (io);
        }

        proc_queue = GetProcQueueByHashID (srv, 
                                           srv->proc_queue_map, 
                                           srv->config->num_process_queues,
                                           buffer);
        
        NetPutRequestBuffer (srv, buffer, proc_queue);
    }
}

DC_INLINE void NetCloseIO (Net_t *net, NetIO_t *io)
{
    if (IOConnected (io)) {
        Dlog ("[libdc] INFO: disconnect remote peer.\n");
        if (net->delegate && net->delegate->willDisconnectWithRemote) {
            net->delegate->willDisconnectWithRemote (net, io);
        }

        IODisconnect (io);
        ev_io_stop (net->ev_loop, &io->ev);
    }

    NetIOClose (io);
}

DC_INLINE void NetAcceptCallBack (struct ev_loop *ev, 
                                    ev_io *w, 
                                    int revents)
{
    NetIO_t *io   = (NetIO_t*)w->data;
    Net_t *srv    = (Net_t*)ev_userdata (ev);
    struct timeval rwtimeo = {0, 0};
    unsigned int szbuf = 0;

    NetIO_t *newio = NetAllocIO (srv);
    if (newio == NULL) {
        //TODO: process exception.
        Dlog ("[libdc] ERROR: can not allocate memory, system is busy.\n");
        return;
    } else {
        NetIOLock (io);
        NetIOInit (newio, io->addr_info);
        
        if (NetIOAcceptRemote (io, newio) < 0) {
            //TODO: process exception.
            NetFreeIO (srv, newio);
            NetIOUnlock (io);
            NetCloseIO (srv, io);
            return;
        } else {
            if (srv->delegate && srv->delegate->willAcceptRemote) {
                if (!srv->delegate->willAcceptRemote(srv, io, newio)) {
                    NetCloseIO (srv, newio);
                    NetFreeIO (srv, newio);
                    return;
                }
            }

            NetIOCtrl (io, NET_IO_CTRL_GET_RECV_TIMEOUT, &rwtimeo, sizeof (rwtimeo));
            NetIOCtrl (newio, NET_IO_CTRL_SET_RECV_TIMEOUT, &rwtimeo, sizeof (rwtimeo));
            NetIOCtrl (io, NET_IO_CTRL_GET_SEND_TIMEOUT, &rwtimeo, sizeof (rwtimeo));
            NetIOCtrl (newio, NET_IO_CTRL_SET_SEND_TIMEOUT, &rwtimeo, sizeof (rwtimeo));
            NetIOCtrl (io, NET_IO_CTRL_GET_RCVBUF, &szbuf, sizeof (szbuf));
            NetIOCtrl (newio, NET_IO_CTRL_SET_RCVBUF, &szbuf, sizeof (szbuf));
            NetIOCtrl (io, NET_IO_CTRL_GET_SNDBUF, &szbuf, sizeof (szbuf));
            NetIOCtrl (newio, NET_IO_CTRL_SET_SNDBUF, &szbuf, sizeof (szbuf));

            IOConnect (newio, io);
            newio->timer = srv->timer;
            newio->ev.data = newio;
            ev_io_init (&newio->ev, 
                        NetReadCallBack, 
                        newio->fd, 
                        EV_READ);
            ev_io_start (srv->ev_loop, &newio->ev);
            NetIOUnlock (io);
        }
    }
}


DC_INLINE int CreateSocketIO (Net_t *serv)
{
    int i;
    NetIO_t *io;
    NetConfig_t *netcfg = serv->config;
    struct timeval rwtimeo =  {(int)(netcfg->rw_timeout/1000), 
                               (netcfg->rw_timeout % 1000)*1000};
    int            flag = 1;

    serv->net_addr_array = (NetAddr_t*)calloc (serv->config->num_sockets, sizeof (NetAddr_t));
    if (serv->net_addr_array == NULL) {
        return -1;
    }

    serv->net_io = (NetIO_t*)calloc (serv->config->num_sockets, sizeof (NetIO_t));
    if (serv->net_io == NULL) {
        return -1;
    }

    for (i=0; i<serv->config->num_sockets &&
        serv->delegate &&
        serv->delegate->getNetAddressWithIndex; i++) {
        io = &serv->net_io[i];

        serv->delegate->getNetAddressWithIndex (serv, &serv->net_addr_array[i], i);
        NetIOInit (io, &serv->net_addr_array[i]);

        if (NetIOCreate (io) < 0) {
            return -1;
        } else {
            if (netcfg->rw_timeout) {
                NetIOCtrl (io, NET_IO_CTRL_SET_RECV_TIMEOUT, &rwtimeo, sizeof (rwtimeo));
                NetIOCtrl (io, NET_IO_CTRL_SET_SEND_TIMEOUT, &rwtimeo, sizeof (rwtimeo));
            }

            if (netcfg->max_sockbuf_size) {
                NetIOCtrl (io, NET_IO_CTRL_SET_SNDBUF, &netcfg->max_sockbuf_size, sizeof (int));
                NetIOCtrl (io, NET_IO_CTRL_SET_RCVBUF, &netcfg->max_sockbuf_size, sizeof (int));
            }

            if (io->addr_info->net_flag & NET_F_BIND) {
                NetIOCtrl (io, NET_IO_CTRL_REUSEADDR, &flag, sizeof (flag));
                if (NetIOBind (io, NULL, 0) < 0) {
                    NetIOClose (io);
                    return -1;
                } else {
                    if (serv->delegate && serv->delegate->didBindToLocal) {
                        serv->delegate->didBindToLocal (serv, io);
                    }
                }
            } else {
                if (NetIOConnect (io, NULL, 0) < 0) {
                    NetIOClose (io);
                    return -1;
                } else {
                    if (serv->delegate && serv->delegate->didBindToLocal) {
                        serv->delegate->didBindToLocal (serv, io);
                    }
                }
            }

            if (NetIOWillAcceptRemote (io)) {
                ev_io_init (&io->ev, NetAcceptCallBack, io->fd, EV_READ);
            } else {
                ev_io_init (&io->ev, NetReadCallBack, io->fd, EV_READ);
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
                       *tmplink= NULL;
    DC_link_t          expired_link;
    int i = 0;

    // check connections for each IO.
    for (i=0; i<net->config->num_sockets; i++) {
        local_io = &net->net_io[i];
        if (!(local_io->addr_info->net_flag & NET_F_BIND)) {
            continue;
        }

        expired_link.next = NULL;
        expired_link.prev = NULL;

        NetIOLock (local_io);
        // loop each connected client IO and to check which IO is 
        // the lastest connected IO.
        linkptr = local_io->PRI (conn_link).prev;
        while (linkptr && linkptr != &local_io->PRI (conn_link)) {
            io = DC_link_container_of (linkptr, NetIO_t, PRI (conn_link));
            if (net->timer - io->timer < net->config->conn_timeout) {
                break;
            }

            tmplink = linkptr->prev;
            DC_link_remove (linkptr);
            DC_link_add    (&expired_link, linkptr);

            linkptr = tmplink;
        }

        // if no clients are found disconnected then go to next.
        NetIOUnlock (local_io);
        if (expired_link.next == NULL) {
            continue;
        }

        // close each disconnected IO.
        linkptr = expired_link.next;
        while (linkptr && linkptr != &expired_link) {
            tmplink = linkptr->next;
            io = DC_link_container_of (linkptr, NetIO_t, PRI (conn_link));
            //NetIORelease (io);
            NetCloseIO (net, io);
            NetFreeIO (net, io);
            linkptr = tmplink;
        }
    }
}

DC_INLINE void DestroySocketIO (Net_t *serv)
{
    int i;

    for (i=0; i<serv->config->num_sockets &&
                serv->net_io; i++) {
        NetIOClose (((NetIO_t*)&serv->net_io[i]));
        ev_io_stop (serv->ev_loop, &serv->net_io[i].ev);
    }

    if (serv->net_io) free (serv->net_io);
    if (serv->net_addr_array) free (serv->net_addr_array);
}

DC_INLINE void WritePID (const char *path, pid_t pid)
{
    FILE *fp = NULL;

    fp = fopen (path, "w");
    if (fp == NULL) {
        Dlog ("[libdc] WARN: can not create PID file [%s].\n", ERRSTR);
        return;
    }

    fprintf (fp, "%u", pid);

    fclose (fp);
}

DC_INLINE void SetLog (const char *logpath)
{
    FILE *log = NULL;

    log = fopen (logpath, "w");
    if (log == NULL) {
        Dlog ("[libdc] WARN: can not create %s log file.\n", ERRSTR);
        return;
    }

    setlinebuf (log);
    dup2 (fileno (log), fileno (stderr));
    dup2 (fileno (log), fileno (stdout));
    dup2 (fileno (log), fileno (stdin));

    fclose (log);
}

DC_INLINE int InitProcQueueMap (NetProcQueue_t *map, 
                                int num, 
                                int num_threads, 
                                unsigned int queue_size)
{
    int i;

    memset (map, '\0', num*sizeof (NetProcQueue_t));

    for (i=0; i<num; i++) {
        if (DC_mutex_init (&map[i].lock, 0, NULL, NULL) < 0) {
            Dlog ("[libdc] ERROR: initialized mutex failed at line:%d.\n", __LINE__);
            return -1;
        }

        if (DC_thread_pool_manager_init (&map[i].proc_pool, num_threads, NULL) < 0) {
            Dlog ("[libdc] ERROR: initialized thread pool manager failed at line:%d.\n", __LINE__);
            return -1;
        }

        if (DC_thread_init (&map[i].manager_thread, NULL, NULL) < 0 ||
            DC_thread_init (&map[i].reply_thread, NULL, NULL) < 0) {
            Dlog ("[libdc] ERROR: allocate threads failed at line: %d.\n", __LINE__);
            return -1;
        }

        DC_queue_init (&map[i].request_queue, queue_size, 0);
        DC_queue_init (&map[i].reply_queue, queue_size, 0);
    }

    return 0;
}

DC_INLINE void DestroyProcQueueMap (NetProcQueue_t *map, int num)
{
    int i;

    for (i=0; i<num; i++) {
        DC_mutex_destroy (&map[i].lock);
        DC_thread_pool_manager_destroy (&map[i].proc_pool);
        DC_queue_destroy (&map[i].request_queue);
        DC_queue_destroy (&map[i].reply_queue);
        DC_thread_destroy (&map[i].manager_thread);
        DC_thread_destroy (&map[i].reply_thread);
    }
    free (map);
}


DC_INLINE int InitNet (Net_t *serv)
{
    NetConfig_t *config     = serv->config;
    NetDelegate_t *delegate = serv->delegate;
    char          strtime[100] = {0};
    char          logpath[500] = {0};
    pid_t         pid;

    Dlog ("[libdc] INFO: STARTING ... ...\n");
    srandom (time (NULL));

    if (config->daemon) {
        pid = fork ();
        if (pid > 0) {
            exit (0);
        } else if (pid < 0) {
            Dlog ("[libdc] WARN: can not fork sub process due to %s.\n", ERRSTR);
            return -1;
        }
    }

    if (config->chdir) chdir (config->chdir);
    if (config->pidfile) WritePID (config->pidfile, getpid ());

    if (config->log ) {
        __NOW ("%Y-%m-%d %T", strtime, sizeof (strtime)-1);
        snprintf (logpath, sizeof (logpath)-1, "%s-%s.log", config->log, strtime);
        SetLog (logpath);
    }

    if (delegate && delegate->willInitNet && delegate->willInitNet (serv) < 0) {
        Dlog ("[libdc] ERROR: initialized system failed.\n");
        return -1;
    }

    if ((config->num_sockbufs && DC_buffer_pool_init (&serv->net_buffer_pool, 
                             config->num_sockbufs,
                             config->max_sockbuf_size+sizeof (NetBuffer_t)) < 0) ||
        (config->num_sock_conns && DC_buffer_pool_init (&serv->net_io_pool,
                                                        config->num_sock_conns,
                                                        sizeof (NetIO_t)) < 0)) {
        Dlog ("[libdc] ERROR: DC_buffer_pool_init failed at line:%d.\n", __LINE__);
        return -1;
    }

    if (DC_mutex_init (&serv->PRI(serv_lock), 0, NULL, NULL) < 0) {
        Dlog ("[libdc] ERROR: DC_mutex_init failed at line:%d.\n", __LINE__);
        return -1;
    }

    if (!((serv->proc_queue_map = (NetProcQueue_t*)calloc (config->num_process_queues, 
                                                         sizeof (NetProcQueue_t))) &&
        !InitProcQueueMap (serv->proc_queue_map, 
                           config->num_process_queues,
                           config->num_threads_each_queue,
                           config->process_queue_size))) {
        Dlog ("[libdc] ERROR: can not initailze process queue map at line: %d.\n", __LINE__);
        return -1;
    }
    
        
    if (DC_thread_init (&serv->conn_checker, NULL, NULL) < 0) {
        Dlog ("[libdc] ERROR: DC_thread_init failed at line:%d.\n", __LINE__);
        return -1;
    }
    serv->ev_loop = ev_loop_new (0);
    ev_set_userdata (serv->ev_loop, serv);

    return 0;
}

DC_INLINE void TimerCallBack (struct ev_loop *ev, ev_timer *w, int revents)
{
    Net_t *srv = (Net_t*)ev_userdata (ev);
    NetBuffer_t *buf = NULL;
    int         i = 0;
    NetIO_t     *io = NULL;
    NetProcQueue_t *queue = NULL;
    srv->timer++;
    
    for (i=0; i<srv->config->num_sockets; i++) {
        io = NetGetIO (srv, i);
        if (srv->config->conn_timeout && io->addr_info->net_flag & NET_F_BIND) {
            if (DC_thread_get_status (((DC_thread_t*)&srv->conn_checker)) \
                == THREAD_STAT_IDLE) {
                DC_thread_run (&srv->conn_checker, CheckNetIOConn, srv);
            }
        } else if (!(io->addr_info->net_flag & NET_F_BIND)) {
            if (!(buf = NetAllocBuffer (srv))) {
                Dlog ("[libdc] WARN: out of memory at line: %d\n", __LINE__);
                continue;
            } else {
                buf->io = io;
                buf->buffer_addr = io->local_addr;
                if (srv->delegate && srv->delegate->ping) {
                    if (srv->delegate->ping (srv, buf)) {
                        queue = GetProcQueueByHashID (srv, 
                                                      srv->proc_queue_map,
                                                      srv->config->num_process_queues,
                                                      buf);
                        NetPutReplyBuffer (srv, buf, queue);
                    }
                }
                NetFreeBuffer (srv, buf);
            }
        }
    }

    if (srv->delegate && srv->delegate->didReceiveTimer) {
        srv->delegate->didReceiveTimer (srv, srv->timer);
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
        Dlog ("[libdc] ERROR: CreateSocketIO failed due to %s\n", ERRSTR);
        return -1;
    }

    timer.data = serv;
    ev_timer_init (&timer, TimerCallBack, serv->config->timer_interval, 1);
    ev_timer_start (serv->ev_loop, &timer);

    sig_handler.data = serv;
    ev_signal_init (&sig_handler, SignalCallBack, SIGINT);
    ev_signal_start(serv->ev_loop, &sig_handler);

    ev_loop (serv->ev_loop, 0);

    if (serv->delegate && serv->delegate->willCloseNet) {
        serv->delegate->willCloseNet (serv);
    }

    ev_timer_stop (serv->ev_loop, &timer);
    ev_signal_stop (serv->ev_loop, &sig_handler);

    DestroySocketIO (serv);

    return 0;
}

DC_INLINE void ReleaseNet (Net_t *serv)
{
    Dlog ("[libdc] INFO: QUITING ... ...\n");
    NetLockContext (serv);
    ev_loop_destroy (serv->ev_loop);
    DestroyProcQueueMap (serv->proc_queue_map, 
                         serv->config->num_process_queues);
    DC_buffer_pool_destroy (&serv->net_io_pool);
    DC_buffer_pool_destroy (&serv->net_buffer_pool);
    DC_mutex_destroy (&serv->PRI(serv_lock));
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
