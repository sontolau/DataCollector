#include "N_net.h"

DC_INLINE void __NK_lock (NetKit *kit)
{
    DC_locker_lock (&kit->locker, LOCK_IN_NORMAL, 1);
}

DC_INLINE void __NK_unlock (NetKit *kit)
{
    DC_locker_unlock (&kit->locker);
}

DC_INLINE DC_object_t *__NK_alloc_buffer_cb (void *data)
{
    NetKit *nk = (NetKit*)data;
    DC_buffer_t *buf = NULL;
    NKBuffer    *nkbuf = NULL;

    if (nk->config->num_sockbufs > 0) {
        buf = DC_buffer_pool_alloc (&nk->buffer_pool,
                                    nk->config->max_sockbuf_size);
        if (buf) {
            nkbuf = (NKBuffer*)buf->buffer;
        }
    } else {
        nkbuf = (NKBuffer*)calloc (1, sizeof (NKBuffer)+nk->config->max_sockbuf_size);
    }

    if (nkbuf) {
        nkbuf->peer = NULL;
        nkbuf->size = nk->config->max_sockbuf_size;
        nkbuf->length = 0;
        nkbuf->pointer = NULL;
        memset (nkbuf->buffer, '\0', nk->config->max_sockbuf_size);
    }

    return (DC_object_t*)nkbuf;
}

DC_INLINE void __NK_release_buffer_cb (DC_object_t *obj, void *data)
{
    NetKit *nk = (NetKit*)data;
    DC_buffer_t *buf = NULL;

    if (kit->config->num_sockbufs > 0) {
        buf = DC_buffer_from (obj);
        if (buf->peer) {
            DC_object_release ((DC_object_t*)buf->peer);
        }
        DC_buffer_pool_free (&kit->buffer_pool, buf);
    } else {
        free (obj);
    }
}

DC_INLINE DC_object_t *__NK_alloc_peer_cb (void *data)
{
    NetKit *nk = (NetKit*)data;
    DC_buffer_t *buf = NULL;
    NKPeer      *peer = NULL;

    if (nk->config->num_sock_conns > 0) {
        buf = DC_buffer_pool_alloc (&nk->peer_pool,
                                    sizeof (NKPeer));
        if (buf) {
            peer = (NKPeer*)buf->buffer;
        }
    } else {
        peer = (NKPeer*)calloc (1, sizeof (NKPeer));
    }

    return (DC_object_t*)peer;
}

DC_INLINE void __NK_release_peer_cb (DC_object_t *obj, void *data)
{
    NetKit *nk = (NetKit*)data;
    DC_buffer_t *buf = NULL;
    NKPeer *peer = (NKPeer*)obj;

    if (nk->config->num_sock_conns > 0) {
        buf = DC_buffer_from (obj);
        DC_buffer_pool_free (&nk->peer_pool, buf);
    } else {
        free (obj);
    }
}

DC_INLINE void __NK_write_pid (const char *path, pid_t pid)
{
    FILE *fp = NULL;

    fp = fopen (path, "w");
    if (fp == NULL) {
        return;
    }

    fprintf (fp, "%u", pid);
    fclose (fp);
}

DC_INLINE void __NK_config_log (const char *logpath)
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


DC_INLINE void __NK_accept_new_peer (NetKit *nk, NKPeer *peer, NKPeer *newpeer)
{
    DC_INLINE void __NK_rdwr_callback();
    newpeer->to = peer;
    ev_io_init (&newpeer->ev_io, __NK_rdwr_callback, newpeer->io->fd, EV_READ);
    newpeer->ev_io.ev.data = newpeer;

    ev_io_start (nk->ev_loop, &newpeer->ev_io);
    DC_list_init (&newpeer->sub_peers, NULL, NULL, NULL);
    DC_list_add_object (&peer->sub_peers, &newpeer->handle);
}

DC_INLINE void __NK_close_peer (NetKit *nk, NKPeer *peer)
{
    if (peer->io) {
        NetIODestroy (peer->io);
        if (peer->to) {
            DC_list_remove_object (&peer->to->sub_peers, &peer->handle);
        }
        ev_io_stop (nk->ev_loop, &peer->ev_io);
        DC_list_destroy (&peer->sub_peers);
    }
}

/////////////////// SDK API /////////////////////////////////
DC_INLINE NKBuffer *__NK_alloc_buffer (NetKit *nk)
{
    return (NKBuffer*)DC_object_alloc (sizeof (NKBuffer),
                                       "NKBuffer",
                                       nk,
                                       __NK_alloc_buffer_cb,
                                       __NK_release_buffer_cb);
}

void __NK_release_buffer (NKBuffer *buf)
{
    DC_object_release ((DC_object_t*)buf);
}

NKPeer *__NK_alloc_peer (NetKit *nk)
{
    return (NKPeer*)DC_object_alloc (sizeof (NKPeer),
                                     "NKPeer",
                                     kit,
                                     __NK_alloc_peer_cb,
                                     __NK_release_peer_cb);

}

NKPeer void __NK_peer_release (NKPeer *peer)
{
    DC_object_release ((DC_object_t*)peer);
}