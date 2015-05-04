#define BufferAlloc(srv, bufptr, type, pool, sz) \
    do {\
        DC_buffer_t *__buf = NULL;\
        __buf = DC_buffer_pool_alloc (&pool, sz);\
        bufptr = (__buf?((type*)__buf->data):NULL);\
    } while (0)

#define BufferFree(srv, buf, pool) \
    do {\
        DC_buffer_t *__buf = (DC_buffer_t*)DC_buffer_from (buf);\
        DC_buffer_pool_free (&pool, __buf);\
    } while (0)

NetBuffer_t *NetAllocBuffer (Net_t *srv)
{
    NetBuffer_t *buf = NULL;

    if (srv->config->num_sockbufs) {
        NetLockContext (srv);
        BufferAlloc (srv, buf, NetBuffer_t, srv->net_buffer_pool, srv->config->max_sockbuf_size);
        if (buf) {
            if (srv->delegate->resourceUsage) {
                srv->delegate->resourceUsage (srv, 
                                RES_BUFFER, 
                                DC_buffer_pool_get_usage (&srv->net_buffer_pool));
            }
        }
        NetUnlockContext (srv);
    } else {
        buf = (NetBuffer_t*)calloc (1, 
              sizeof (NetBuffer_t) + srv->config->max_sockbuf_size);
    }

    if (buf) {
        buf->io            = NULL;
        buf->size   = srv->config->max_sockbuf_size;
        buf->data_length = 0;
        buf->offset = 0;
        buf->buffer = (unsigned char*)buf+sizeof (NetBuffer_t);
        memset (buf->buffer, '\0', buf->size);
    }
    Dlog ("[libdc] allocate system buffer: %p\n", buf);

    return buf;
}

void NetFreeBuffer (Net_t *srv, NetBuffer_t *buf)
{
    if (srv->config->num_sockbufs) {
        NetLockContext (srv);
        BufferFree (srv, buf, srv->net_buffer_pool);
        if (srv->delegate->resourceUsage) {
            srv->delegate->resourceUsage (srv, 
                          RES_BUFFER, 
                          DC_buffer_pool_get_usage (&srv->net_buffer_pool));
        }
        NetUnlockContext (srv);
    } else {
        free (buf);
    }

    Dlog ("[libdc] free system buffer: %p\n", buf);
}

NetIO_t *NetAllocIO (Net_t *srv)
{
    NetIO_t *iobuf = NULL;

    if (srv->config->num_sock_conns) {
        NetLockContext (srv);
        BufferAlloc (srv, iobuf, NetIO_t, srv->net_io_pool, sizeof (NetIO_t));
        if (iobuf) {
            memset (iobuf, '\0', sizeof (NetIO_t));
            if (srv->delegate->resourceUsage) {
                srv->delegate->resourceUsage (srv, 
                                          RES_IO, 
                                          DC_buffer_pool_get_usage (&srv->net_io_pool));
            }
        }
        NetUnlockContext (srv);
    } else {
       iobuf = (NetIO_t*)calloc (1, sizeof (NetIO_t));
    }
  
    Dlog ("[libdc] allocate IO : %p\n", iobuf);
    return iobuf;
}

/*
void NetIORelease (NetIO_t *io)
{
    io->refcount--;
    if (io->refcount <= 0) {
        DC_mutex_destroy (&io->io_lock);
        if (io->release_cb) {
            io->release_cb (io, io->cb_data);
        }
    }
}

NetIO_t *NetIOGet (NetIO_t *io)
{
    io->refcount++;
    return io;
}
*/

void NetFreeIO (Net_t *srv, NetIO_t *io)
{
    if (srv->config->num_sock_conns) {
        NetLockContext (srv);
        BufferFree (srv, io, srv->net_io_pool);
        if (srv->delegate->resourceUsage) {
            srv->delegate->resourceUsage (srv, 
                                   RES_BUFFER, 
                                   DC_buffer_pool_get_usage (&srv->net_io_pool));
        }
        NetUnlockContext (srv);
    } else {
        free (io);
    }

    Dlog ("[libdc] free IO: %p\n", io);
}

/*
void NetBufferSetIO (Net_t *srv, NetBuffer_t *buf, NetIO_t *io)
{
    if (io) {
        buf->io = NetIOGet(io);
        DC_link_add (io->PRI (buffer_link).prev, &buf->PRI (buffer_link));
    }
}

void NetBufferRemoveIO (Net_t *srv, NetBuffer_t *buf)
{
    if (buf->io) {
        NetIORelease (buf->io);
        DC_link_remove (&buf->PRI (buffer_link));
        buf->io = NULL;
    }
}*/
