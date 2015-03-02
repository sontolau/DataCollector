
DC_INLINE void SetNonblockFD (int fd)
{
    int flag = 0;

    if ((flag = fcntl (fd, F_GETFL)) < 0) {
        return;
    }

    flag |= O_NONBLOCK;

    fcntl (fd, F_SETFL, &flag);
}

#define BufferAlloc(srv, bufptr, type, pool, sz) \
    do {\
        DC_buffer_t *__buf = NULL;\
        DC_mutex_lock (&srv->buf_lock, 0, 1);\
        __buf = DC_buffer_pool_alloc (&pool, sz);\
        DC_mutex_unlock (&srv->buf_lock);\
        bufptr = (__buf?((type*)__buf->data):NULL);\
    } while (0)

#define BufferFree(srv, buf, pool) \
    do {\
        DC_buffer_t *__buf = (DC_buffer_t*)DC_buffer_from (buf);\
        DC_mutex_lock (&srv->buf_lock, 0, 1);\
        DC_buffer_pool_free (&pool, __buf);\
        DC_mutex_unlock (&srv->buf_lock);\
    } while (0)

NetBuffer_t *NetBufferAlloc (Net_t *srv)
{
    NetBuffer_t *buf = NULL;
    if (srv->config->max_buffers) {
        BufferAlloc (srv, buf, NetBuffer_t, srv->net_buffer_pool, srv->config->buffer_size);
        if (buf) {
            memset (buf->buffer, '\0', srv->config->buffer_size);
            buf->refcount = 1;
        } else {
            return NULL;
        }

        if (srv->delegate->resourceUsage) {
            srv->delegate->resourceUsage (srv, 
                                RES_BUFFER, 
                                DC_buffer_pool_get_usage (&srv->net_buffer_pool));
        }
    } else {
        if ((buf = (NetBuffer_t*)calloc (1, 
                   sizeof (NetBuffer_t) + srv->config->buffer_size))) {
            buf->refcount = 1;
        }
    }

    return buf;
}

void NetBufferFree (Net_t *srv, NetBuffer_t *buf)
{
    buf->refcount--;
    if (buf->refcount <= 0) {
        if (srv->config->max_buffers) {
            BufferFree (srv, buf, srv->net_buffer_pool);
            if (srv->delegate->resourceUsage) {
                srv->delegate->resourceUsage (srv, 
                              RES_BUFFER, 
                              DC_buffer_pool_get_usage (&srv->net_buffer_pool));
            }
        } else {
            free (buf);
        }
    }
    //fprintf (stderr, "Free:%p\n", buf);
}

NetIO_t *NetIOAlloc (Net_t *srv)
{
    NetIO_t *iobuf = NULL;
    
    if (srv->config->max_requests) {
        BufferAlloc (srv, iobuf, NetIO_t, srv->net_io_pool, sizeof (NetIO_t));
        if (iobuf) {
            memset (iobuf, '\0', sizeof (NetIO_t));
            iobuf->refcount = 1;
        } else {
            return NULL;
        }

        if (srv->delegate->resourceUsage) {
            srv->delegate->resourceUsage (srv, 
                                          RES_IO, 
                                          DC_buffer_pool_get_usage (&srv->net_io_pool));
        }
    } else {
        if ((iobuf = (NetIO_t*)calloc (1, sizeof (NetIO_t)))) {
            iobuf->refcount = 1;
        }
    }
    iobuf->PRI (buf_link).next = \
    iobuf->PRI (buf_link).prev = \
    &iobuf->PRI (buf_link);

    DC_mutex_init (&iobuf->PRI (io_lock), 0, NULL, NULL);
    return iobuf;
}

void NetIOFree (Net_t *srv, NetIO_t *io)
{
    NetIOLock (io);
    io->refcount--;
    NetIOUnlock (io);

    if (io->refcount <= 0) {
        DC_mutex_destroy (&io->PRI (io_lock));
        if (srv->config->max_requests) {
            BufferFree (srv, io, srv->net_io_pool);
            if (srv->delegate->resourceUsage) {
                srv->delegate->resourceUsage (srv, 
                                          RES_BUFFER, 
                                          DC_buffer_pool_get_usage (&srv->net_io_pool));
            }
        } else {
            free (io);
        }
    }
}


NetIO_t *NetIOGet (NetIO_t *io)
{
    NetIOLock (io);
    io->refcount++;
    NetIOUnlock (io);
    return io;
}

NetBuffer_t *NetBufferGet (NetBuffer_t *buf)
{
    buf->refcount++;
    return buf;
}
