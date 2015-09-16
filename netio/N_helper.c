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
    NetKit *kit = (NetKit*)data;
    DC_buffer_t *buf = NULL;
    NKBuffer    *nkbuf = NULL;

    if (kit->config->num_sockbufs > 0) {
        buf = DC_buffer_pool_alloc (&kit->buffer_pool,
                                    kit->config->max_sockbuf_size);
        if (buf) {
            nkbuf = (NKBuffer*)buf->buffer;
            nkbuf->size = kit->config->max_sockbuf_size;
            nkbuf->length = 0;
            memset (nkbuf->buffer, '\0', kit->config->max_sockbuf_size);
        }
    } else {
        nkbuf = (NKBuffer*)calloc (1, sizeof (NKBuffer)+kit->config->max_sockbuf_size);
    }


    return (DC_object_t*)nkbuf;
}

DC_INLINE void __NK_release_buffer_cb (DC_object_t *obj, void *data)
{
    NetKit *kit = (NetKit*)data;
    DC_buffer_t *buf = NULL;

    if (kit->config->num_sockbufs > 0) {
        buf = DC_buffer_from (obj);
        DC_buffer_pool_free (&kit->buffer_pool, buf);
    } else {
        free (obj);
    }
}

DC_INLINE DC_object_t *__NK_alloc_peer_cb (void *data)
{
    NetKit *kit = (NetKit*)data;
    DC_buffer_t *buf = NULL;
    NKPeer      *peer = NULL;

    if (kit->config->num_sock_conns > 0) {
        buf = DC_buffer_pool_alloc (&kit->peer_pool,
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
    NetKit *kit = (NetKit*)data;
    DC_buffer_t *buf = NULL;
    NKPeer *peer = (NKPeer*)obj;

    if (kit->config->num_sock_conns > 0) {
        buf = DC_buffer_from (obj);
        DC_buffer_pool_free (&kit->peer_pool, buf);
    } else {
        free (obj);
    }
}

DC_INLINE void __NK_close_peer (NKPeer *peer)
{
    NetIODestroy (&peer->io);
    ev_io_stop (&peer->ev_io);
    DC_list_destroy (&peer->peer_list);
    if (peer->to) {
        DC_list_remove_object (&peer->to->peer_list, &peer->list_handle);
    }
}


/////////////////// SDK API /////////////////////////////////
NKBuffer *NK_buffer_alloc (NetKit *kit)
{
    return (NKBuffer*)DC_object_alloc (sizeof (NKBuffer),
                                       "NKBuffer",
                                       kit,
                                       __NK_alloc_buffer_cb,
                                       __NK_release_buffer_cb);
}

void NK_buffer_release (NKBuffer *buf)
{
    DC_object_release ((DC_object_t*)buf);
}

NKPeer *NK_peer_alloc (NetKit *kit)
{
    NKPeer *peer;


    peer = (NKPeer*)DC_object_alloc (sizeof (NKPeer),
                                     "NKPeer",
                                     kit,
                                     __NK_alloc_peer_cb,
                                     __NK_release_buffer_cb);

    return peer;
}

void NK_peer_release (NKPeer *peer)
{
    DC_object_release ((DC_object_t*)peer);
}