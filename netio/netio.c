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
#include "netio.h"
#include "net/nettcp.c"
#include "net/netudp.c"
#include "net/netcomm.c"


#define IOConnect(_from, _to) \
    do {\
        DC_link_add (&_to->PRI (peer_link), &_from->PRI (peer_link));\
        _from->to = _to;\
    } while (0)

#define IOIsConnected(_io) (_io->to)

#define IODisconnect(_io) \
    do {\
        if (_io->to) {\
            DC_link_remove (&_io->PRI (peer_link));\
            _io->to = NULL;\
        }\
    } while (0)

#define IOUpdate(_io) \
    do {\
        DC_link_remove (&_io->PRI (peer_link));\
        DC_link_add (&_io->PRI (peer_link), &_io->to->PRI (peer_link));\
    } while (0)

#define IORelease(_net, _io) \
    do {\
        _io->PRI(ref_count)--;\
        if (_io->PRI(ref_count) <= 0) {\
            if (IOIsConnected (_io)) {\
                Dlog ("[libdc] INFO: disconnect from remote .\n");\
                IODisconnect (io);\
                ev_io_stop (net->ev_loop, &io->ev);\
            }\
            CB (net, willCloseRemote, net, io);\
            NetIODestroy (io);\
        }\
    } while (0)


#define CB(_net, _func, ...) \
    do {\
        if (_net->delegate && _net->delegate->_func) {\
            _net->delegate->_func (__VA_ARGS__); \
        }\
    } while (0);

#define CB2(_retval, _net, _func, ...) \
    do {\
        if (_net->delegate && _net->delegate->_func) {\
            _retval = _net->delegate->_func (__VA_ARGS__);\
        }\
    } while (0)

#define DGT(x) x->delegate

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


int NetIOInit (NetIO_t *io,
               const NetAddrInfo_t *info)
{
    if (info) {
        io->__handler = &NetProtocolHandler[info->net_type];
        io->addr_info = (NetAddrInfo_t*)info;
    }

    io->PRI(ref_count) = 1;
    DC_mutex_init (&io->io_lock, 0, NULL, NULL);
    DC_link_init2 (io->PRI (peer_link), NULL);
    return 0;
}

static void ProcBufferHandler (NetQueueManager_t *mgr,
                               qobject_t obj,
                               void *data)
{
    Net_t *net = (Net_t*)data;
    NetBuffer_t *buf = (NetBuffer_t*)obj;
    NetIO_t *io = buf->io;

    assert (buf->io);
    NetBufferDettachIO (buf);
    CB (net, processData, net, io, buf);
    NetFreeBuffer (net, buf);
}


static void ReplyBufferHandler (NetQueueManager_t *mgr,
                                qobject_t obj,
                                void *data)
{
    Net_t *net = (Net_t*)data;
    NetBuffer_t *buf= (NetBuffer_t*)obj;
    long    status = 1;
    
    assert (buf->io);

    NetBufferDettachIO (buf);
    status = NetIOWriteTo (buf->io, buf);
    CB (net, didWriteData, net, buf->io, buf, status?errno:0);
    if (status < 0) {
        IORelease (net, buf->io);
    }
}

void NetAddIncomingData (Net_t *net, const NetBuffer_t *buf, NetQueueManager_t *queue)
{
    RunTaskInQueue (queue, (qobject_t)buf);
}

void NetWrite (Net_t *net, const NetBuffer_t *buf)
{
    RunTaskInQueue (&net->reply_queue, (qobject_t)buf);
}

DC_INLINE NetQueueManager_t *GetProcQueueByHashID (Net_t *net,
                                                NetQueueManager_t *map,
                                                int num,
                                                NetBuffer_t *buf)
{
    unsigned int i = -1;
    unsigned int hash_id = 0;
    NetQueueManager_t *mgrptr = NULL;

    CB2 (i, net, getHashQueue, net, buf);
    if (i >= 0) {
        return &map[i%num];
    }

    //TODO: generate a long ID with struct sockaddr type.
/*
    if (buf->io->addr_info->net_flag & NET_F_IPv6) {
        for (i=0; i<sizeof (buf->buffer_addr.s6.sin6_addr); i++) {
            hash_id += ((char*)&buf->buffer_addr.s6.sin6_addr)[i];
        }
    } else {
        hash_id = buf->buffer_addr.s4.sin_addr.s_addr;
    }
*/
    return &map[hash_id % num];
}

DC_INLINE void NetReadCallBack (struct ev_loop *ev, ev_io *w, int revents)
{
    Net_t *srv = (Net_t*)ev_userdata (ev);
    NetIO_t *io = (NetIO_t*)w->data;
    NetBuffer_t *buffer = NULL;
    NetQueueManager_t *proc_queue = NULL;

    // allocate a buffer and IO to receive data.
    if (!(buffer = NetAllocBuffer (srv))) {
        //TODO: process exception.
        return;
    }

    NetIOLock (io);
    buffer->io = IOGet(io);
    io->PRI(ref_count)++; 
    // read data.
    if ((int)(buffer->data_length = NetIOReadFrom (buffer->io, buffer)) <= 0) {
        //TODO:
        NetFreeBuffer (srv, buffer);
        NetIOUnlock (io);
        IORelease (srv, io);
        NetIOUnlock (io);
        NetFreeIO (io);
        return;
    } else {
        Dlog ("[libdc] INFO: received %u bytes.\n", buffer->data_length);
        CB (srv, didReadData, srv, buffer->io, buffer);
        // put data buffer into queue and wake up to process.
        if (io->to) {
            io->timer = srv->timer;
            IOUpdate (io);
        }

        proc_queue = GetProcQueueByHashID (srv, 
                                           srv->proc_queue_map, 
                                           srv->config->num_process_queues,
                                           buffer);
        NetIOUnlock (io);
        NetAddIncomingData (srv, buffer, proc_queue);
    }
}

void NetCloseIO (Net_t *net, NetIO_t *io)
{
    NetIOLock (io);
    IORelease (net, io);
    NetIOUnlock (io);
}

DC_INLINE void NetAcceptCallBack (struct ev_loop *ev, 
                                    ev_io *w, 
                                    int revents)
{
    NetIO_t *io   = (NetIO_t*)w->data;
    Net_t *srv    = (Net_t*)ev_userdata (ev);
    //struct timeval rwtimeo = {0, 0};
    unsigned int szbuf = 0;
    int           flag = 1;

    NetIO_t *newio = NetAllocIO (srv);
    if (newio == NULL) {
        //TODO: process exception.
        Dlog ("[libdc] ERROR: can not allocate memory, system is busy.\n");
        return;
    } else {
        NetIOInit (newio, io->addr_info);
        NetIOLock (io);
        
        if (NetIOAcceptRemote (io, newio) < 0) {
            //TODO: process exception.
            IORelease (srv, newio);
            NetIOUnlock (io);
            return;
        } else {
            CB2 (flag, srv, willAcceptRemote, srv, io, newio);
            if (!flag) {
                IORelease (srv, newio);
                return;
            }

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
            Dlog ("[Libdc] INFO: a new client has been accpted.\n");
        }
    }
}


DC_INLINE int CreateSocketIO (Net_t *serv)
{
    int i;
    NetIO_t *io;
    NetConfig_t *netcfg = serv->config;
    int            flag = 1;

    serv->net_addr_array = (NetAddrInfo_t*)calloc (serv->config->num_sockets, sizeof (NetAddrInfo_t));
    if (serv->net_addr_array == NULL) {
        return -1;
    }

    serv->net_io = (NetIO_t*)calloc (serv->config->num_sockets, sizeof (NetIO_t));
    if (serv->net_io == NULL) {
        return -1;
    }

    for (i=0; i<serv->config->num_sockets; i++) {
        io = &serv->net_io[i];

        CB (serv, getNetAddressInfo, serv, &serv->net_addr_array[i], i);
        NetIOInit (io, &serv->net_addr_array[i]);

        if (NetIOCreate (io) < 0) {
            return -1;
        } else {
            NetIOCtrl (io, NET_IO_CTRL_SET_NONBLOCK, NULL, 0);

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
                    CB (serv, didBindToLocal, serv, io);
                }
            } else {
                if (NetIOConnect (io) < 0) {
                    NetIOClose (io);
                    return -1;
                } else {
                    CB (serv, didBindToLocal, serv, io);
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

#define WATCH_DOG_TIMEOUT(x) (x & 0x00FF)
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
        linkptr = local_io->PRI (peer_link).prev;
        while (linkptr && linkptr != &local_io->PRI (peer_link)) {
            io = DC_link_container_of (linkptr, NetIO_t, PRI (peer_link));
            if (net->timer - io->timer < WATCH_DOG_TIMEOUT (net->config->watch_dog)) {
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
            io = DC_link_container_of (linkptr, NetIO_t, PRI (peer_link));
            Dlog ("[libdc] INFO: a client will be disconnected due to timeout of transaction.\n");
            IORelease (net, io);
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
        NetIODestroy (((NetIO_t*)&serv->net_io[i]));
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

    log = fopen (logpath, "w+");
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

DC_INLINE int InitQueueManagers (Net_t *net)
{
    int i;

    net->proc_queue_map = (NetQueueManager_t*)calloc (net->config->num_process_queues,
                                                      sizeof (NetQueueManager_t));

    for (i=0; i<net->config->num_process_queues; i++) {
        InitQueueManager (&net->proc_queue_map[i], 
                          net->config->process_queue_size, 
                          net->config->num_threads_each_queue, 
                          ProcBufferHandler, 
                          net);
    }

    InitQueueManager (&net->reply_queue,
                      net->config->process_queue_size, 
                      net->config->num_threads_each_queue, 
                      ReplyBufferHandler, 
                      net);


    return 0;
}

DC_INLINE void DestroyQueueManagers (Net_t *net)
{
    int i;

    for (i=0; i<net->config->num_process_queues; i++) {
        DestroyQueueManager (&net->proc_queue_map[i]);
    }

    DestroyQueueManager (&net->reply_queue);
    free (net->proc_queue_map);
}


DC_INLINE int InitNet (Net_t *serv)
{
    NetConfig_t *config     = serv->config;
    NetDelegate_t *delegate = serv->delegate;
    pid_t         pid;
    int           ret = -1;

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

    if (config->log) {
        SetLog (config->log);
    }

    CB2 (ret, serv, willInitNet, serv);
    if (ret < 0) {
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

    if (InitQueueManagers (serv) < 0) {
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
    //NetBuffer_t *buf = NULL;
    int         i = 0;
    NetIO_t     *io = NULL;
 
    srv->timer++;
    
    for (i=0; i<srv->config->num_sockets; i++) {
        io = NetGetIO (srv, i);
        if (srv->config->watch_dog && io->addr_info->net_flag & NET_F_BIND) {
            if (DC_thread_get_status (((DC_thread_t*)&srv->conn_checker)) \
                == THREAD_STAT_IDLE) {
                DC_thread_run (&srv->conn_checker, CheckNetIOConn, srv);
            }
        }
        /* else if (!(io->addr_info->net_flag & NET_F_BIND)) {
            if (!(buf = NetAllocBuffer (srv))) {
                Dlog ("[libdc] WARN: out of memory at line: %d\n", __LINE__);
                continue;
            } else {
                NetBufferSetIO (buf, io);
                buf->buffer_addr = io->local_addr;
                if (srv->delegate && srv->delegate->ping) {
                    if (srv->delegate->ping (srv, buf)) {
                        NetCommitIO (srv, io);

                    }
                }
                NetFreeBuffer (srv, buf);
            }
        }*/
    }

    CB (srv, didReceiveTimer, srv, srv->timer);
}

DC_INLINE void SignalCallBack (struct ev_loop *ev, ev_signal *w, int revents)
{
    ev_break (ev, EVBREAK_ALL);
}

#define WATCH_DOG_DELAY(x) ((x & 0xFF00)>>8)
DC_INLINE int RunNet (Net_t *serv)
{
    ev_timer  timer;
    ev_signal sig_handler;

    if (CreateSocketIO (serv) < 0) {
        Dlog ("[libdc] ERROR: CreateSocketIO failed due to %s\n", ERRSTR);
        return -1;
    }

    timer.data = serv;
    ev_timer_init (&timer, TimerCallBack, 1, WATCH_DOG_DELAY(serv->config->watch_dog));
    ev_timer_start (serv->ev_loop, &timer);

    sig_handler.data = serv;
    ev_signal_init (&sig_handler, SignalCallBack, SIGINT);
    ev_signal_start(serv->ev_loop, &sig_handler);

    ev_loop (serv->ev_loop, 0);

    CB (serv, willCloseNet, serv);
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
    DestroyQueueManagers (serv);
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
