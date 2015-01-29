
DC_INLINE void SetNonblockFD (int fd)
{
    int flag = 0;

    if ((flag = fcntl (fd, F_GETFL)) < 0) {
        return;
    }

    flag |= O_NONBLOCK;

    fcntl (fd, F_SETFL, &flag);
}

#ifdef _USE_STATIC_BUFFER

#define BufferAlloc(srv, bufptr, type, pool) \
    do {\
        DC_buffer_t *__buf = NULL;\
        DC_mutex_lock (&srv->buf_lock, 0, 1);\
        __buf = DC_buffer_pool_alloc (&pool, srv->config->buffer_size);\
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
# else

#endif

NetBuffer_t *NetBufferAlloc (Net_t *srv)
{
    NetBuffer_t *buf = NULL;
#ifdef _USE_STATIC_BUFFER
    BufferAlloc (srv, buf, NetBuffer_t, srv->net_buffer_pool);
    if (buf) {
        memset (buf->buffer, '\0', srv->config->buffer_size);
    }

    if (srv->delegate->resourceUsage) {
        srv->delegate->resourceUsage (srv, 
                    RES_BUFFER, 
                    DC_buffer_pool_get_usage (&srv->net_buffer_pool));
    }
#else
    buf = (NetBuffer_t*)calloc (1, 
          sizeof (NetBuffer_t) + srv->config->buffer_size);
#endif
    //fprintf (stderr, "Allocate: %p\n", buf);
    return buf;
}

void NetBufferFree (Net_t *srv, NetBuffer_t *buf)
{
#ifdef _USE_STATIC_BUFFER
    BufferFree (srv, buf, srv->net_buffer_pool);
    if (srv->delegate->resourceUsage) {
        srv->delegate->resourceUsage (srv, 
                      RES_BUFFER, 
                      DC_buffer_pool_get_usage (&srv->net_buffer_pool));
    }
#else
    free (buf);
#endif
    //fprintf (stderr, "Free:%p\n", buf);
}

NetIO_t *NetIOAlloc (Net_t *srv)
{
    NetIO_t *iobuf = NULL;
#ifdef _USE_STATIC_BUFFER

    BufferAlloc (srv, iobuf, NetIO_t, srv->net_io_pool);
    if (srv->delegate->resourceUsage) {
        srv->delegate->resourceUsage (srv, 
                                      RES_IO, 
                                      DC_buffer_pool_get_usage (&srv->net_io_pool));
    }
#else
    iobuf = (NetIO_t*)calloc (1, sizeof (NetIO_t));
#endif
    return iobuf;
}

void NetIOFree (Net_t *srv, NetIO_t *io)
{
#ifdef _USE_STATIC_BUFFER
    BufferFree (srv, io, srv->net_io_pool);
    if (srv->delegate->resourceUsage) {
        srv->delegate->resourceUsage (srv, 
                                      RES_BUFFER, 
                                      DC_buffer_pool_get_usage (&srv->net_io_pool));
    }
#else
    free (io);
#endif
}



