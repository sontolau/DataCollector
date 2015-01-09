#ifndef _NETUTIL_H
#define _NETUTIL_H

#include <libdc/libdc.h>
#include "netsrv.h"
#include <libdc/buffer.h>
#include <fcntl.h>

DC_CPP (extern "C" {)


DC_INLINE void SetNonblockFD (int fd)
{
    int flag = 0;

    if ((flag = fcntl (fd, F_GETFL)) < 0) {
        return;
    }

    flag |= O_NONBLOCK;

    fcntl (fd, F_SETFL, &flag);
}


#define BufferAlloc(srv, bufptr, type, pool) \
    do {\
        DC_buffer_t *__buf = NULL;\
        pthread_mutex_lock (&srv->buf_lock);\
        __buf = DC_buffer_pool_alloc (&pool, srv->config->buffer_size);\
        pthread_mutex_unlock (&srv->buf_lock);\
        bufptr = (__buf?((type*)__buf->data):NULL);\
    } while (0)

#define BufferFree(srv, buf, pool) \
    do {\
        DC_buffer_t *__buf = (DC_buffer_t*)DC_buffer_from (buf);\
        pthread_mutex_lock (&srv->buf_lock);\
        DC_buffer_pool_free (&pool, __buf);\
        pthread_mutex_unlock (&srv->buf_lock);\
    } while (0)


NetBuffer_t *NetBufferAlloc (Net_t *srv)
{
    NetBuffer_t *buf = NULL;
    BufferAlloc (srv, buf, NetBuffer_t, srv->net_buffer_pool);
    if (buf) {
        memset (buf->buffer, '\0', srv->config->buffer_size);
    }

    if (srv->delegate->resourceUsage) {
        srv->delegate->resourceUsage (srv, 
                    RES_BUFFER, 
                    DC_buffer_pool_get_usage (&srv->net_buffer_pool));
    }

    //fprintf (stderr, "Allocate: %p\n", buf);
    return buf;
}

void NetBufferFree (Net_t *srv, NetBuffer_t *buf)
{
    BufferFree (srv, buf, srv->net_buffer_pool);
    if (srv->delegate->resourceUsage) {
        srv->delegate->resourceUsage (srv, 
                      RES_BUFFER, 
                      DC_buffer_pool_get_usage (&srv->net_buffer_pool));
    }
    //fprintf (stderr, "Free:%p\n", buf);
}

NetIO_t *NetIOAlloc (Net_t *srv)
{
    NetIO_t *iobuf = NULL;

    BufferAlloc (srv, iobuf, NetIO_t, srv->net_io_pool);
    if (srv->delegate->resourceUsage) {
        srv->delegate->resourceUsage (srv, 
                                      RES_IO, 
                                      DC_buffer_pool_get_usage (&srv->net_io_pool));
    }
    return iobuf;
}

void NetIOFree (Net_t *srv, NetIO_t *io)
{
    BufferFree (srv, io, srv->net_io_pool);
    if (srv->delegate->resourceUsage) {
        srv->delegate->resourceUsage (srv, 
                                      RES_BUFFER, 
                                      DC_buffer_pool_get_usage (&srv->net_io_pool));
    }
}



DC_CPP (})
#endif

