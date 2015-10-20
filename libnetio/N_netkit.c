#include "N_netkit.h"
#include "N_helper.c"

DC_INLINE void __NK_reply_cb (void *userdata, void *data)
{
    NetKit *nk = userdata;
    NKBuffer *buf = data;

    NetBufSetBuffer (buf->skbuf, buf->buffer, buf->length);
    buf->length = NetIOWrite (buf->peer->io, &buf->skbuf);
    if (buf->length < 0) {
        if (nk->delegate && nk->delegate->didFailToSendData) {
            nk->delegate->didFailToSendData (nk, buf->peer, buf);
        }
    } else {
        if (nk->delegate && nk->delegate->didSuccessToSendData) {
            nk->delegate->didSuccessToSendData (nk, buf->peer, buf);
        }
    }

    NK_free_buffer (nk, buf);
}

DC_INLINE void __NK_process_cb (void *userdata, void *data)
{
    NetKit *nk = userdata;
    NKBuffer *buf = (NKBuffer*)data;

    if (nk->delegate && nk->delegate->processData) {
        nk->delegate->processData (nk, buf->peer, buf);
    }

    NK_free_buffer (nk, buf);
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
    NetBufSetBuffer (buffer->skbuf, buffer->buffer, buffer->size);
    buffer->length = NetIORead (peer->io, &buffer->skbuf);
    peer->last_counter = nk->counter;

    if (peer->to) {
        DC_list_remove_object (&peer->to->sub_peers, &peer->handle);
        DC_list_insert_object_at_index (&peer->to->sub_peers, &peer->handle, 0);
    }
    DC_locker_unlock (&nk->locker);

    if (buffer->length < 0) {
        if (nk->delegate && nk->delegate->didFailToReceiveData) {
            nk->delegate->didFailToReceiveData (nk, peer);
        }
        NK_free_buffer (nk, buffer);
    } else {
        if (nk->delegate && nk->delegate->didSuccessToReceiveData) {
            nk->delegate->didSuccessToReceiveData (nk, peer, buffer);
        }

        DC_task_queue_run_task (&nk->process_task_queue, (void*)buffer, 1);
    }
}

DC_INLINE void __NK_accept_callback (struct ev_loop *ev, ev_io *w, int revents)
{
    NKPeer *peer = w->data;
    NKPeer *new_peer = NULL;
    NetKit *nk  = ev_userdata (ev);

    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    new_peer = __NK_alloc_peer (nk);
    if (new_peer) {
        if (!NetIOAccept (peer->io, new_peer->io)) {
            new_peer->last_counter = nk->counter;
            __NK_accept_new_peer (nk, peer, new_peer);
        } else {
            __NK_release_peer (new_peer);
            new_peer = NULL;
        }
    }
    DC_locker_unlock (&nk->locker);
}

DC_INLINE void __NK_check_callback (void *data)
{
    NetKit *nk = (NetKit*)data;
    DC_list_elem_t *ele = NULL;
    void *saveptr = NULL;
    NKPeer *peer = NULL, *subpeer = NULL;
    DC_list_t   tmplist;

    DC_list_init (&tmplist, NULL, NULL, NULL);
    while (1) {
        DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
        ele = DC_list_next_object (&nk->infaces, &saveptr);
        if (!ele) { DC_locker_unlock (&nk->locker); break;}

        peer = CONTAINER_OF (ele, NKPeer, handle);
        while (peer->sub_peers.count > 0 && 
               (ele = DC_list_get_object_at_index (&peer->sub_peers, 
                                               peer->sub_peers.count - 1))) {
            subpeer = CONTAINER_OF (ele, NKPeer, handle);
            if (nk->counter - subpeer->last_counter 
                > WATCH_DOG_TIMEOUT (nk->config->watch_dog)) {
                subpeer->to = NULL;
                DC_list_remove_object_at_index (&peer->sub_peers, peer->sub_peers.count-1);
                DC_list_add_object (&tmplist, ele);
            } else {
                break;
            }
        }
        DC_locker_unlock (&nk->locker);
    }
    
    while (tmplist.count > 0 && (ele = DC_list_get_object_at_index (&tmplist, 0))) {
        peer = CONTAINER_OF (ele, NKPeer, handle);
        DC_list_remove_object_at_index (&tmplist, 0);
        NK_close_peer (nk, peer);
    }
}

DC_INLINE void __NK_timer_callback (struct ev_loop *ev, ev_timer *w, int revents)
{
    NetKit *nk = ev_userdata (ev);
    unsigned int timeout = WATCH_DOG_TIMEOUT (nk->config->watch_dog);

    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    nk->counter++;
    if (nk->delegate && nk->delegate->didReceiveTimer) {
        nk->delegate->didReceiveTimer (nk);
    }
    DC_locker_unlock (&nk->locker);
    if (timeout && !(nk->counter % timeout)) {
        DC_thread_run (&nk->checker_thread, __NK_check_callback, nk);
    }
}

DC_INLINE void __NK_signal_callback (struct ev_loop *ev, ev_signal *w, int revents)
{
    NetKit *nk = ev_userdata (ev);

    if (nk->delegate && nk->delegate->didReceiveSignal) {
        nk->delegate->didReceiveSignal (nk, revents);
    }
}


int NK_init (NetKit *nk, const NKConfig *cfg)
{
    memset (nk, '\0', sizeof (NetKit));

    nk->config = (NKConfig*)cfg;
    srandom (time (NULL));

    if (DC_thread_init (&nk->checker_thread) != ERR_OK) {
        return -1;
    }

/*
    if (DC_thread_init (&nk->proc_thread) != ERR_OK) {
        return -1;
    }

    if (DC_thread_init (&nk->reply_thread) != ERR_OK) {
        return -1;
    }


    if (DC_queue_init (&nk->request_queue, cfg->queue_size) != ERR_OK) {
        return -1;
    }

    if (DC_queue_init (&nk->reply_queue, cfg->queue_size) != ERR_OK) {
        return -1;
    }

*/
    if (DC_task_queue_init (&nk->process_task_queue, cfg->queue_size, cfg->num_process_threads, __NK_process_cb, nk) < 0 ||
        DC_task_queue_init (&nk->reply_task_queue, cfg->queue_size, cfg->num_reply_threads, __NK_reply_cb, nk) < 0) {
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


    if (DC_locker_init (&nk->locker, 1, NULL) != ERR_OK) {
        return -1;
    }

    return 0;
}

int __NK_run (NetKit *nk)
{
    ev_timer timer;
    ev_signal signal;
    NKConfig       *cfg  = nk->config;

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
    ev_set_userdata (nk->ev_loop, nk);
    ev_loop (nk->ev_loop, 0);
    ev_signal_stop (nk->ev_loop, &signal);
    ev_timer_stop (nk->ev_loop, &timer);

    return 0;
}

void NK_set_userdata (NetKit *nk, const void *data)
{
    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    nk->private_data = (void*)data;
    DC_locker_unlock (&nk->locker);
}

void NK_set_delegate (NetKit *nk, NKDelegate *delegate)
{
    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    nk->delegate = delegate;
    DC_locker_unlock (&nk->locker);
}

NKBuffer *NK_alloc_buffer (NetKit *nk)
{
    NKBuffer *buf;

    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    buf = __NK_alloc_buffer (nk);
    DC_locker_unlock (&nk->locker);

    return buf;
}

NKBuffer *NK_buffer_get (NKBuffer *buf)
{
    return (NKBuffer*)DC_object_get ((DC_object_t*)buf);
}

void NK_buffer_set_peer (NKBuffer *buf, NKPeer *peer)
{
    if (peer) {
        buf->peer = (NKPeer*)DC_object_get ((DC_object_t*)peer);
    } else {
        if (buf->peer) {
            DC_OBJECT_RELEASE ((DC_object_t*)buf->peer);
            buf->peer = NULL;
        }
    }
}
/*
INetAddress_t *NK_buffer_get_inet_addr (NKBuffer *buf)
{
    return NetBufGetINetAddr (((NetBuf_t*)&buf->skbuf));
}

void NK_buffer_set_inet_addr (NKBuffer *buf, INetAddress_t *addr)
{
    NetBufSetINetAddr (((NetBuf_t*)&buf->skbuf), addr);
}

int NK_buffer_set_data (NKBuffer *buf, void *data, long szdata)
{
    if (szdata > buf->skbuf_size) return -1;

    buf->skbuf.bufptr = data;
    buf->skbuf.szbuf  = szdata;

    return 0;
}
*/
NKPeer *NK_alloc_peer (NetKit *nk)
{
    NKPeer *peer = NULL;

    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    peer = __NK_alloc_peer (nk);
    DC_locker_unlock (&nk->locker);

    return peer;
}

NKPeer *NK_peer_get (NKPeer *peer)
{
    return (NKPeer*)DC_object_get ((DC_object_t*)peer);
}

void NK_peer_free (NetKit *nk, NKPeer *peer)
{
    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    __NK_release_peer (peer);
    DC_locker_unlock (&nk->locker);
}

void NK_free_buffer (NetKit *nk, NKBuffer *buf)
{
    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    __NK_release_buffer (buf);
    DC_locker_unlock (&nk->locker);
}

void NK_free_peer (NetKit *nk, NKPeer *peer)
{
    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    __NK_release_peer (peer);
    DC_locker_unlock (&nk->locker);
}

void NK_close_peer (NetKit *nk, NKPeer *peer)
{
    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    __NK_close_peer (nk, peer);
    DC_locker_unlock (&nk->locker);
}

int NK_commit_buffer (NetKit *nk, NKBuffer *buf)
{
    void __NK_reply_callback (void*);
    int ret = ERR_OK;


    if (!(ret = DC_task_queue_run_task (&nk->reply_task_queue, (void*)buf, 1))) {
        NK_buffer_get (buf);
    }

    return ret;
}

int NK_commit_bulk_buffers (NetKit *nk, NKBuffer **buf, int num)
{
    int i;


    for (i=0;  i<num; i++) {
        if (NK_commit_buffer (nk, buf[i]) == ERR_OK) {
            i++;
        } else {
            break;
        }
    }

    return i;
}

void *NK_get_userdata (NetKit *nk) 
{
    void *data = NULL;

    DC_locker_lock (&nk->locker, LOCK_IN_READ, 1);
    data = nk->private_data;
    DC_locker_unlock (&nk->locker);

    return data;
}

DC_INLINE int NKPeer_init (DC_object_t *obj, void *data)
{
    return 0;
}

DC_INLINE void NKPeer_release (DC_object_t *obj, void *data)
{
    DC_object_release (obj);
}

int NK_add_netio (NetKit *nk, NetIO_t *io, int ev)
{
    int ev_bits = 0;
/*
    NKPeer *peer = (NKPeer*)DC_object_alloc (sizeof (NKPeer),
                                    "NKPeer",
                                    NULL,
                                    NULL,
                                    NULL);
*/
    NKPeer *peer = DC_OBJECT_NEW (NKPeer, NULL);
    DC_locker_lock (&nk->locker, LOCK_IN_WRITE, 1);
    peer->io = io;
    peer->to = NULL;
    peer->last_counter = nk->counter;

    if (ev == NK_EV_ACCEPT) {
        ev_io_init (&peer->ev_io, __NK_accept_callback, io->fd, EV_READ);
    } else {
        if (ev & NK_EV_READ) {
            ev_bits |= EV_READ;
        }
        ev_io_init (&peer->ev_io, __NK_rdwr_callback, io->fd, ev_bits);
    }
    peer->ev_io.data = peer;
    ev_io_start (nk->ev_loop, &peer->ev_io);
    DC_list_init (&peer->sub_peers, NULL, NULL, NULL);
    DC_list_add_object (&nk->infaces, &peer->handle);
    DC_locker_unlock (&nk->locker);

    return 0;
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
            ev_io_stop (nk->ev_loop, &peer->ev_io);
            DC_list_destroy (&peer->sub_peers);
            DC_list_remove_object (&nk->infaces, &peer->handle);
            DC_OBJECT_RELEASE ((DC_object_t*)peer);
            break;
        } else if (!io) {
            ev_io_stop (nk->ev_loop, &peer->ev_io);
            DC_list_destroy (&peer->sub_peers);
            DC_list_remove_object (&nk->infaces, &peer->handle);
            DC_OBJECT_RELEASE ((DC_object_t*)peer);
        }
    }
    DC_locker_unlock (&nk->locker);
}

DC_INLINE void __NK_destroy (NetKit *nk)
{
    Dlog ("[NetKit] INFO: QUITING ... ...\n");

    DC_task_queue_destroy (&nk->process_task_queue);
    DC_task_queue_destroy (&nk->reply_task_queue);

    /*
    DC_thread_destroy (&nk->proc_thread);
    DC_thread_destroy (&nk->reply_thread);
    DC_queue_destroy (&nk->request_queue);
    DC_queue_destroy (&nk->reply_queue);
    */

    NK_remove_netio (nk, NULL);
    ev_loop_destroy (nk->ev_loop);
    DC_list_destroy (&nk->infaces);
    DC_buffer_pool_destroy (&nk->io_pool);
    DC_buffer_pool_destroy (&nk->buffer_pool);
    DC_buffer_pool_destroy (&nk->peer_pool);

    DC_locker_destroy (&nk->locker);
}

int NK_run (NetKit *nk)
{
    return __NK_run (nk);
}

void NK_destroy (NetKit *nk)
{
    __NK_destroy (nk);
}

void NK_stop (NetKit *nk)
{
    ev_break (nk->ev_loop, EVBREAK_ALL);
}
