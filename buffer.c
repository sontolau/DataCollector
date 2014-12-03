#include "buffer.h"

int BufferPoolInit (BufferPool_t *pool, int num, unsigned int size)
{
    int i = 0;
    register Buffer_t *bufptr = NULL;

    memset (pool, '\0', sizeof (BufferPool_t));

    pool->__netbuf = (Buffer_t*)calloc (num, size + sizeof (Buffer_t));
    if (pool->__netbuf == NULL) {
        return -1;
    }

    bufptr = pool->__netbuf;
    for (i=0; i<num; i++) {

        bufptr->size   = size;
        bufptr->length = 0;

        DC_link_add (&pool->__free_link, &bufptr->__link);
        bufptr = (Buffer_t*)((unsigned char*)bufptr + (size + sizeof (Buffer_t)));
    }

    return 0;
}

Buffer_t *BufferAlloc (BufferPool_t *pool)
{
    DC_link_t *linkptr = NULL;
    Buffer_t  *netbuf  = NULL;

    linkptr = pool->__free_link.next;
    if (linkptr) {
        DC_link_remove (linkptr);
        DC_link_add (&pool->__engaged_link, linkptr);

        netbuf = DC_link_container_of (linkptr, Buffer_t, __link);
    }

    return netbuf;
}

void     BufferFree (BufferPool_t *pool, Buffer_t *buf)
{
    buf->length = 0;
    buf->size   = 0;

    DC_link_remove (&buf->__link);
    DC_link_add    (&pool->__free_link, &buf->__link);
}

void BufferPoolDestroy (BufferPool_t *pool)
{
    free (pool->__netbuf);
}

