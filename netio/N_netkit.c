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

#include "N_netkit.h"
#include "N_helper.c"

//
//int NetIOInit (NetIO_t *io,
//               const NetAddrInfo_t *info)
//{
//    if (info) {
//        io->__handler = &NetProtocolHandler[info->net_type];
//        io->addr_info = (NetAddrInfo_t*)info;
//    }
//
//    io->PRI(ref_count) = 1;
//    DC_mutex_init (&io->io_lock, 0, NULL, NULL);
//    DC_link_init2 (io->PRI (peer_link), NULL);
//    return 0;
//}
//
//static void ProcBufferHandler (NetQueueManager_t *mgr,
//                               qobject_t obj,
//                               void *data)
//{
//    Net_t *net = (Net_t*)data;
//    NetBuffer_t *buf = (NetBuffer_t*)obj;
//    NetIO_t *io = buf->io;
//
//    assert (buf->io);
//    NetBufferDettachIO (buf);
//    CB (net, processData, net, io, buf);
//    NetFreeBuffer (net, buf);
//}
//
//
//static void ReplyBufferHandler (NetQueueManager_t *mgr,
//                                qobject_t obj,
//                                void *data)
//{
//    Net_t *net = (Net_t*)data;
//    NetBuffer_t *buf= (NetBuffer_t*)obj;
//    long    status = 1;
//
//    assert (buf->io);
//
//    NetBufferDettachIO (buf);
//    status = NetIOWriteTo (buf->io, buf);
//    CB (net, didWriteData, net, buf->io, buf, status?errno:0);
//    if (status < 0) {
//        IORelease (net, buf->io);
//    }
//}
//
//void NetAddIncomingData (Net_t *net, const NetBuffer_t *buf, NetQueueManager_t *queue)
//{
//    RunTaskInQueue (queue, (qobject_t)buf);
//}
//
//void NetWrite (Net_t *net, const NetBuffer_t *buf)
//{
//    RunTaskInQueue (&net->reply_queue, (qobject_t)buf);
//}
//
//DC_INLINE NetQueueManager_t *GetProcQueueByHashID (Net_t *net,
//                                                NetQueueManager_t *map,
//                                                int num,
//                                                NetBuffer_t *buf)
//{
//    unsigned int i = -1;
//    unsigned int hash_id = 0;
//    NetQueueManager_t *mgrptr = NULL;
//
//    CB2 (i, net, getHashQueue, net, buf);
//    if (i >= 0) {
//        return &map[i%num];
//    }
//
//    //TODO: generate a long ID with struct sockaddr type.
///*
//    if (buf->io->addr_info->net_flag & NET_F_IPv6) {
//        for (i=0; i<sizeof (buf->buffer_addr.s6.sin6_addr); i++) {
//            hash_id += ((char*)&buf->buffer_addr.s6.sin6_addr)[i];
//        }
//    } else {
//        hash_id = buf->buffer_addr.s4.sin_addr.s_addr;
//    }
//*/
//    return &map[hash_id % num];
//}
//
//DC_INLINE void NetReadCallBack (struct ev_loop *ev, ev_io *w, int revents)
//{
//    Net_t *srv = (Net_t*)ev_userdata (ev);
//    NetIO_t *io = (NetIO_t*)w->data;
//    NetBuffer_t *buffer = NULL;
//    NetQueueManager_t *proc_queue = NULL;
//
//    // allocate a buffer and IO to receive data.
//    if (!(buffer = NetAllocBuffer (srv))) {
//        //TODO: process exception.
//        return;
//    }
//
//    NetIOLock (io);
//    buffer->io = IOGet(io);
//    io->PRI(ref_count)++;
//    // read data.
//    if ((int)(buffer->data_length = NetIOReadFrom (buffer->io, buffer)) <= 0) {
//        //TODO:
//        NetFreeBuffer (srv, buffer);
//        NetIOUnlock (io);
//        IORelease (srv, io);
//        NetIOUnlock (io);
//        NetFreeIO (io);
//        return;
//    } else {
//        Dlog ("[libdc] INFO: received %u bytes.\n", buffer->data_length);
//        CB (srv, didReadData, srv, buffer->io, buffer);
//        // put data buffer into queue and wake up to process.
//        if (io->to) {
//            io->timer = srv->timer;
//            IOUpdate (io);
//        }
//
//        proc_queue = GetProcQueueByHashID (srv,
//                                           srv->proc_queue_map,
//                                           srv->config->num_process_queues,
//                                           buffer);
//        NetIOUnlock (io);
//        NetAddIncomingData (srv, buffer, proc_queue);
//    }
//}
//
//void NetCloseIO (Net_t *net, NetIO_t *io)
//{
//    NetIOLock (io);
//    IORelease (net, io);
//    NetIOUnlock (io);
//}
//
//DC_INLINE void NetAcceptCallBack (struct ev_loop *ev,
//                                    ev_io *w,
//                                    int revents)
//{
//    NetIO_t *io   = (NetIO_t*)w->data;
//    Net_t *srv    = (Net_t*)ev_userdata (ev);
//    //struct timeval rwtimeo = {0, 0};
//    unsigned int szbuf = 0;
//    int           flag = 1;
//
//    NetIO_t *newio = NetAllocIO (srv);
//    if (newio == NULL) {
//        //TODO: process exception.
//        Dlog ("[libdc] ERROR: can not allocate memory, system is busy.\n");
//        return;
//    } else {
//        NetIOInit (newio, io->addr_info);
//        NetIOLock (io);
//
//        if (NetIOAcceptRemote (io, newio) < 0) {
//            //TODO: process exception.
//            IORelease (srv, newio);
//            NetIOUnlock (io);
//            return;
//        } else {
//            CB2 (flag, srv, willAcceptRemote, srv, io, newio);
//            if (!flag) {
//                IORelease (srv, newio);
//                return;
//            }
//
//            NetIOCtrl (io, NET_IO_CTRL_GET_RCVBUF, &szbuf, sizeof (szbuf));
//            NetIOCtrl (newio, NET_IO_CTRL_SET_RCVBUF, &szbuf, sizeof (szbuf));
//            NetIOCtrl (io, NET_IO_CTRL_GET_SNDBUF, &szbuf, sizeof (szbuf));
//            NetIOCtrl (newio, NET_IO_CTRL_SET_SNDBUF, &szbuf, sizeof (szbuf));
//
//            IOConnect (newio, io);
//            newio->timer = srv->timer;
//            newio->ev.data = newio;
//            ev_io_init (&newio->ev,
//                        NetReadCallBack,
//                        newio->fd,
//                        EV_READ);
//            ev_io_start (srv->ev_loop, &newio->ev);
//            NetIOUnlock (io);
//            Dlog ("[Libdc] INFO: a new client has been accpted.\n");
//        }
//    }
//}
//
//
//DC_INLINE int CreateSocketIO (Net_t *serv)
//{
//    int i;
//    NetIO_t *io;
//    NetConfig_t *netcfg = serv->config;
//    int            flag = 1;
//
//    serv->net_addr_array = (NetAddrInfo_t*)calloc (serv->config->num_sockets, sizeof (NetAddrInfo_t));
//    if (serv->net_addr_array == NULL) {
//        return -1;
//    }
//
//    serv->net_io = (NetIO_t*)calloc (serv->config->num_sockets, sizeof (NetIO_t));
//    if (serv->net_io == NULL) {
//        return -1;
//    }
//
//    for (i=0; i<serv->config->num_sockets; i++) {
//        io = &serv->net_io[i];
//
//        CB (serv, getNetAddressInfo, serv, &serv->net_addr_array[i], i);
//        NetIOInit (io, &serv->net_addr_array[i]);
//
//        if (NetIOCreate (io) < 0) {
//            return -1;
//        } else {
//            NetIOCtrl (io, NET_IO_CTRL_SET_NONBLOCK, NULL, 0);
//
//            if (netcfg->max_sockbuf_size) {
//                NetIOCtrl (io, NET_IO_CTRL_SET_SNDBUF, &netcfg->max_sockbuf_size, sizeof (int));
//                NetIOCtrl (io, NET_IO_CTRL_SET_RCVBUF, &netcfg->max_sockbuf_size, sizeof (int));
//            }
//
//            if (io->addr_info->net_flag & NET_F_BIND) {
//                NetIOCtrl (io, NET_IO_CTRL_REUSEADDR, &flag, sizeof (flag));
//                if (NetIOBind (io, NULL, 0) < 0) {
//                    NetIOClose (io);
//                    return -1;
//                } else {
//                    CB (serv, didBindToLocal, serv, io);
//                }
//            } else {
//                if (NetIOConnect (io) < 0) {
//                    NetIOClose (io);
//                    return -1;
//                } else {
//                    CB (serv, didBindToLocal, serv, io);
//                }
//            }
//
//            if (NetIOWillAcceptRemote (io)) {
//                ev_io_init (&io->ev, NetAcceptCallBack, io->fd, EV_READ);
//            } else {
//                ev_io_init (&io->ev, NetReadCallBack, io->fd, EV_READ);
//            }
//            ev_io_start (serv->ev_loop, &io->ev);
//            io->ev.data = io;
//        }
//    }
//
//    return 0;
//}
//
//#define WATCH_DOG_TIMEOUT(x) (x & 0x00FF)
//DC_INLINE void CheckNetIOConn (DC_thread_t *thread, void *data)
//{
//    Net_t *net = (Net_t*)data;
//    NetIO_t *local_io = NULL,
//            *io= NULL;
//    register DC_link_t *linkptr = NULL,
//                       *tmplink= NULL;
//    DC_link_t          expired_link;
//    int i = 0;
//
//    // check connections for each IO.
//    for (i=0; i<net->config->num_sockets; i++) {
//        local_io = &net->net_io[i];
//        if (!(local_io->addr_info->net_flag & NET_F_BIND)) {
//            continue;
//        }
//
//        expired_link.next = NULL;
//        expired_link.prev = NULL;
//        NetIOLock (local_io);
//        // loop each connected client IO and to check which IO is
//        // the lastest connected IO.
//        linkptr = local_io->PRI (peer_link).prev;
//        while (linkptr && linkptr != &local_io->PRI (peer_link)) {
//            io = DC_link_container_of (linkptr, NetIO_t, PRI (peer_link));
//            if (net->timer - io->timer < WATCH_DOG_TIMEOUT (net->config->watch_dog)) {
//                break;
//            }
//
//            tmplink = linkptr->prev;
//            DC_link_remove (linkptr);
//            DC_link_add    (&expired_link, linkptr);
//
//            linkptr = tmplink;
//        }
//
//        // if no clients are found disconnected then go to next.
//        NetIOUnlock (local_io);
//        if (expired_link.next == NULL) {
//            continue;
//        }
//
//        // close each disconnected IO.
//        linkptr = expired_link.next;
//        while (linkptr && linkptr != &expired_link) {
//            tmplink = linkptr->next;
//            io = DC_link_container_of (linkptr, NetIO_t, PRI (peer_link));
//            Dlog ("[libdc] INFO: a client will be disconnected due to timeout of transaction.\n");
//            IORelease (net, io);
//            linkptr = tmplink;
//        }
//    }
//}

/*
DC_INLINE void __NK_process_data (void *data)
{
    NKBuffer *buf = data;
    NetKit *nk    = buf->pointer;

    nk->delegate->processData (nk, buf->peer, buf);
    NK_buffer_free (nk, buf);
}
*/

DC_INLINE void __NK_reply_callback (void *data)
{
    NetKit *nk = data;
    NKBuffer *buf = NULL;

    do {
        DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
        buf = (NKBuffer*)DC_queue_fetch (&nk->reply_queue);
        DC_locker_unlock (&nk->locker);
        if ((obj_t)buf == QZERO) {
            break;
        } else {
            nk->ev_cb (nk, NULL, NK_EV_WRITE, buf);
            NK_buffer_free (nk, buf);
        }
    } while (1);
}

DC_INLINE void __NK_process_callback (void *data)
{
    NetKit *nk = data;
    NKBuffer *buf = NULL;
    static DC_queue_t *tmp_queue = NULL;

    if (!tmp_queue) {
        tmp_queue = (DC_queue_t*)calloc (1, sizeof (DC_queue_t));
        DC_queue_init (tmp_queue, nk->config->queue_size);
    }

    do {
        DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
        if (DC_queue_is_empty (&nk->request_queue)) {
            DC_locker_unlock (&nk->locker);
            break;
        }
        while ((buf = DC_queue_fetch (&nk->request_queue)) != QZERO) {
            DC_queue_add (tmp_queue, (obj_t)buf);
        }
        DC_locker_unlock (&nk->locker);

        while ((obj_t)(buf = DC_queue_fetch (tmp_queue)) != QZERO) {
            //nk->delegate->processData (nk, buf);
            nk->ev_cb (nk, NULL, NK_EV_PROC, buf);
            NK_buffer_free (nk, buf);
        }
    } while (1);
}

DC_INLINE void __NK_rdwr_callback (struct ev_loop *ev, ev_io *w, int revents)
{
    NKPeer *peer = w->data;
    NetKit *nk  = ev_userdata (ev);
    NKBuffer *buffer = NULL;

    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    buffer = __NK_alloc_buffer (nk);
    if (!buffer) {
        //TODO:
        DC_locker_unlock (&nk->locker);
        return;
    }

    NK_buffer_set_peer (buffer, peer);
    if (!nk->ev_cb (nk, peer, NK_EV_READ, buffer)) {
        buffer->sockaddr = peer->io->sock_addr;
        DC_queue_add (&nk->request_queue, (obj_t)buffer);
    } else {
        __NK_release_buffer (nk, buffer);
    }
    DC_locker_unlock (&nk->locker);
    if (DC_thread_get_status (&nk->proc_thread) != THREAD_STAT_RUNNNG) {
        DC_thread_run (&nk->proc_thread, __NK_process_callback, nk);
    }
}

DC_INLINE void __NK_accept_callback (struct ev_loop *ev, ev_io *w, int revents)
{
    NKPeer *peer = w->data;
    NKPeer *new_peer = NULL;
    NetKit *nk  = ev_userdata (ev);
    int ret = -1;

    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    new_peer = __NK_alloc_peer (nk);
    if (new_peer) {
        if (nk->ev_cb) {
            ret = nk->ev_cb (nk, peer, NK_EV_ACCEPT, new_peer);
            if (!ret) {
                __NK_accept_new_peer (nk, peer, new_peer);
            }
        }

        if (ret) {
            __NK_release_peer (nk, new_peer);
        }
    }
    DC_locker_unlock (&nk->locker);
}

DC_INLINE void __NK_timer_callback (struct ev_loop *ev, ev_timer *w, int revents)
{
    NetKit *nk = ev_userdata (ev);

    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    nk->counter++;
    if (nk->ev_cb) {
        nk->ev_cb (nk, NULL, NK_EV_TIMER, NULL);
    }
    DC_locker_unlock (&nk->locker);

}

DC_INLINE void __NK_signal_callback (struct ev_loop *ev, ev_signal *w, int revents)
{
    ev_break (ev, EVBREAK_ALL);
}


int NK_init (NetKit *nk, const NKConfig *cfg)
{
    memset (nk, '\0', sizeof (NetKit));

    nk->config = cfg;
    srandom (time (NULL));

    /*
    if (delegate->willInit (nk) < 0) {
        return -1;
    }
    */

    if (DC_thread_init (&nk->proc_thread) != ERR_OK) {
        return -1;
    }

    if (DC_thread_init (&nk->reply_thread) != ERR_OK) {
        return -1;
    }

    if (DC_list_init (&nk->infaces, NULL, NULL, NULL) != ERR_OK) {
        return -1;
    }

    if (!(nk->ev_loop = ev_loop_new (0))) {
        return -1;
    }

    if (cfg->num_sock_conns > 0 && (
        DC_buffer_pool_init (&nk->peer_pool,
                             cfg->num_sock_conns,
                             sizeof (NKPeer)) != ERR_OK ||
        DC_buffer_pool_init (&nk->io_pool,
                             cfg->num_sock_conns,
                             sizeof (NetIO_t)) != ERR_OK)) {
        return -1;
    }

    if (cfg->num_sockbufs > 0 &&
        DC_buffer_pool_init (&nk->buffer_pool,
                             cfg->num_sockbufs,
                             sizeof (NKBuffer)+cfg->max_sockbuf_size) != ERR_OK) {
        return -1;
    }

    if (DC_queue_init (&nk->request_queue, cfg->queue_size) != ERR_OK) {
        return -1;
    }

    if (DC_queue_init (&nk->reply_queue, cfg->queue_size) != ERR_OK) {
        return -1;
    }

    if (DC_task_manager_init (&nk->task_manager, cfg->num_process_threads) != ERR_OK) {
        return -1;
    }

    if (DC_locker_init (&nk->locker, 1, NULL) != ERR_OK) {
        return -1;
    }

    return 0;
}

int NK_run (NetKit *nk)
{
    ev_timer timer;
    ev_signal signal;
    void *saveptr = NULL;
    DC_list_elem_t *ele = NULL;
    NKPeer         *peer = NULL;

    if (cfg->daemon) {
        daemon (0, 0);
    }

    if (cfg->chdir) chdir (cfg->chdir);
    if (cfg->pidfile) __NK_write_pid (cfg->pidfile, getpid ());
    if (cfg->log) __NK_config_log (cfg->log);

    timer.data = nk;
    ev_timer_init (&timer, __NK_timer_callback, 1, WATCH_DOG_DELAY(nk->config->watch_dog));
    ev_timer_start (nk->ev_loop, &timer);

    signal.data = nk;
    ev_signal_init (&signal, __NK_signal_callback, SIGINT|SIGTERM|SIGPIPE);
    ev_signal_start(nk->ev_loop, &signal);

    ev_loop (nk->ev_loop, 0);
    ev_signal_stop (nk->ev_loop, &signal);
    ev_timer_stop (nk->ev_loop, &timer);

    return 0;
}

void NK_set_userdata (NetKit *nk const void *data)
{
    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    nk->private_data = (void*)data;
    DC_locker_unlock (&nk->locker);
}

void NK_set_event_callback (NetKit *nk, void (*cb)(NetKit*, NKPeer*, int, void*))
{
    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    nk->ev_cb = cb;
    DC_locker_unlock (&nk->locker);
}

NKBuffer *NK_buffer_alloc (NetKit *nk)
{
    NKBuffer *buf;

    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    buf = __NK_alloc_buffer (nk);
    DC_locker_unlock (&nk->locker);

    return buf;
}

NKBuffer *NK_buffer_get (NetKit *nk)
{
    return DC_object_get ((DC_object_t*)nk);
}

void NK_buffer_set_peer (NKBuffer *buf, NKPeer *peer)
{
    if (peer) {
        buf->peer = (NKPeer*)DC_object_get ((DC_object_t*)peer);
    } else {
        if (buf->peer) {
            DC_object_release ((DC_object_t*)buf->peer);
            buf->peer = NULL;
        }
    }
}

NKPeer *NK_peer_alloc (NetKit *nk)
{
    NKPeer *peer = NULL;

    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    peer = __NK_alloc_peer (nk);
    DC_locker_unlock (&nk->locker);

    return peer;
}

NKPeer *NK_peer_get (NKPeer *peer)
{
    return DC_object_get ((DC_object_t*)peer);
}

void NK_peer_free (NetKit *nk, NKPeer *peer)
{
    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    __NK_release_peer (nk, peer);
    DC_locker_unlock (&nk->locker);
}

void NK_buffer_free (NetKit *nk, NKBuffer *buf)
{
    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    __NK_release_buffer (nk, buf);
    DC_locker_unlock (&nk->locker);
}

void NK_close_peer (NetKit *nk, NKPeer *peer)
{
    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    __NK_close_peer (nk, peer);
    DC_locker_unlock (&nk->locker);
}

void NK_commit_buffer (NetKit *nk, NKBuffer *buf)
{
    void __NK_reply_callback (void*);

    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    DC_queue_add (&nk->reply_queue, buf);
    DC_buffer_get (buf);
    DC_locker_unlock (&nk->locker);

    if (DC_thread_get_status (&nk->reply_thread) != THREAD_STAT_RUNNING) {
        DC_thread_run (&nk->reply_thread, __NK_reply_callback, buf);
    }
}

void NK_commit_buffer_bulk (NetKit *nk, NKBuffer **buf, int num)
{
    int i;

    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    for (i=0;  i<num; i++) {
        DC_queue_add (&nk->reply_queu, (obj_t)buf[i]);
        DC_buffer_get (buf[i]);
    }
    DC_locker_unlock (&nk->locker);

    if (DC_thread_get_status (&nk->reply_thread) != THREAD_STAT_RUNNING) {
        DC_thread_run (&nk->reply_thread, __NK_reply_callback, buf);
    }
}

void *NK_get_userdata (NetKit *nk) {
    void *data = NULL;

    DC_locker_lock (&nk->locker, LOCK_IN_READ, 1);
    data = nk->private_data;
    DC_locker_unlock (&nk->locker);

    return data;
}

int NK_set_netio (NetKit *nk, NetIO_t *io, int ev)
{
    int ev_bits = 0;

    NKPeer *peer = DC_object_alloc (sizeof (NKPeer),
                                    "NKPeer",
                                    NULL,
                                    NULLL,
                                    NULL);

    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    peer->io = io;
    peer->to = NULL;
    peer->last_counter = 0;

    if (ev == NK_EV_ACCEPT) {
        ev_io_init (&peer->ev_io, __NK_accept_callback, io->fd, EV_READ);
    } else {
        if (ev & NK_EV_READ) {
            ev_bits |= EV_READ;
        }
        if (ev & NK_EV_WRITE) {
            ev_bits |= EV_WRITE;
        }

        ev_io_init (&peer->ev_io, __NK_rdwr_callback, io->fd, ev_bits);
    }
    peer->ev_io.ev.data = &peer;

    ev_io_start (nk->ev_loop, &peer->ev_io);
    DC_list_init (&peer->sub_peers, NULL, NULL, NULL);
    DC_list_add_object (&nk->infaces, &peer->handle);
    DC_locker_unlock (&nk->locker)
}

void NK_remove_netio (NetKit *nk, NetIO_t *io)
{
    DC_list_elem_t *ele = NULL;
    NKPeer *peer = NULL;
    void *saveptr = NULL;

    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    while ((ele = DC_list_next_object (&nk->infaces, &saveptr))) {
        peer = CONTAINER_OF (ele, NKPeer, handle);
        if (io && peer->io == io) {
            ev_io_stop (&peer->ev_io);
            DC_list_destroy (&peer->sub_peers);
            DC_list_remove_object (&nk->infaces, &peer->handle);
            DC_object_release ((DC_object_t*)peer);
            break;
        } else if (!io) {
            ev_io_stop (&peer->ev_io);
            DC_list_destroy (&peer->sub_peers);
            DC_list_remove_object (&nk->infaces, &peer->handle);
            DC_object_release ((DC_object_t*)peer);
        }
    }
    DC_locker_unlock (&nk->locker);
}

DC_INLINE void __NK_destroy (NetKit *nk)
{
    Dlog ("[NetKit] INFO: QUITING ... ...\n");

    DC_thread_destroy (&nk->proc_thread);
    DC_thread_destroy (&nk->reply_thread);
    NK_remove_io (nk, NULL);
    ev_loop_destroy (nk->ev_loop);
    DC_list_destroy (&nk->infaces);
    DC_buffer_pool_destroy (&nk->io_pool);
    DC_buffer_pool_destroy (&nk->buffer_pool);
    DC_buffer_pool_destroy (&nk->peer_pool);
    DC_queue_destroy (&nk->request_queue);
    DC_queue_destroy (&nk->reply_queue);
    DC_task_manager_destroy (&nk->taks_manager);
    DC_locker_destroy (&nk->locker);
    /*
    NetLockContext (serv);
    ev_loop_destroy (serv->ev_loop);
    DestroyQueueManagers (serv);
    DC_buffer_pool_destroy (&serv->net_io_pool);
    DC_buffer_pool_destroy (&serv->net_buffer_pool);
    DC_mutex_destroy (&serv->PRI(serv_lock));
    */
}

int NK_run (NetKit *nk, const NKConfig *config, const NKDelegate *delegate)
{
    __NK_run (nk);
    __NK_destroy (nk);

    return 0;
}

void NK_stop (NetKit *nk)
{
    kill (0, SIGINT);
}
