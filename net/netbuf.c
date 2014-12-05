#include "netsrv.h"

#define BufferAlloc(srv, bufptr, type, pool) \
        DC_buffer_t *buf = NULL;\
        pthread_rwlock_wrlock (&srv->serv_lock);\
        buf = DC_buffer_pool_alloc (&pool);\
        pthread_rwlock_unlock (&srv->serv_lock);\
        bufptr = (buf?((type*)buf->data):NULL);

#define BufferFree(srv, buf, pool) \
        pthread_rwlock_wrlock (&srv->serv_lock);\
        DC_buffer_pool_free (&pool, DC_buffer_pool_from (buf));\
        pthread_rwlock_unlock (&srv->serv_lock);

NetPeer_t *NetPeerAlloc (Server_t *srv)
{
    NetPeer_t  *peer = NULL;
    DC_buffer_t *buf;

    BufferAlloc (srv, peer, NetPeer_t, srv->net_peer_pool);
    if (peer) {
        memset (peer, '\0', sizeof (NetPeer_t));
        buf = DC_buffer_pool_from (peer);
        peer->cid = buf->id;
        peer->net_io = NULL;
    }

    return peer;
}

void NetPeerFree (Server_t *srv, NetPeer_t *peer)
{
    BufferFree (srv, peer, srv->net_peer_pool);
}

NetIO_t *NetIOAlloc (Server_t *srv)
{
    NetIO_t *io = NULL;

    BufferAlloc (srv, io, NetIO_t, srv->net_io_pool);
    if (io) {
        memset (io, '\0', sizeof (NetIO_t));
    }

    return io;
}

void NetIOFree (Server_t *srv, NetIO_t *io)
{
    BufferFree (srv, io, srv->net_io_pool);
}

NetBuffer_t *NetBufferAlloc (Server_t *srv)
{
    NetBuffer_t *buf = NULL;

    BufferAlloc (srv, buf, NetBuffer_t, srv->net_buffer_pool);
    if (buf) {
        memset (buf, '\0', sizeof (NetBuffer_t));
        buf->buffer_size = srv->config->buffer_size;
    }

    return buf;
}

void NetBufferFree (Server_t *srv, NetBuffer_t *buf)
{
    BufferFree (srv, buf, srv->net_buffer_pool);
}
