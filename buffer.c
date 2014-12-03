#include "buffer.h"

int DC_buffer_pool_init (DC_buffer_pool_t *pool, int num, unsigned int size)
{
    int i = 0;
    register DC_buffer_t *bufptr = NULL;

    memset (pool, '\0', sizeof (DC_buffer_pool_t));

    pool->__netbuf = (DC_buffer_t*)calloc (num, size + sizeof (DC_buffer_t));
    if (pool->__netbuf == NULL) {
        return -1;
    }

    bufptr = pool->__netbuf;
    for (i=0; i<num; i++) {

        bufptr->size   = size;
        bufptr->length = 0;

        DC_link_add (&pool->__free_link, &bufptr->__link);
        bufptr = (DC_buffer_t*)((unsigned char*)bufptr + (size + sizeof (DC_buffer_t)));
    }

    return 0;
}

DC_buffer_t *DC_buffer_pool_alloc (DC_buffer_pool_t *pool)
{
    DC_link_t *linkptr = NULL;
    DC_buffer_t  *netbuf  = NULL;

    linkptr = pool->__free_link.next;
    if (linkptr) {
        DC_link_remove (linkptr);
        DC_link_add (&pool->__engaged_link, linkptr);

        netbuf = DC_link_container_of (linkptr, DC_buffer_t, __link);
    }

    return netbuf;
}

void     DC_buffer_pool_free (DC_buffer_pool_t *pool, DC_buffer_t *buf)
{
    buf->length = 0;
    buf->size   = 0;

    DC_link_remove (&buf->__link);
    DC_link_add    (&pool->__free_link, &buf->__link);
}

void DC_buffer_pool_destroy (DC_buffer_pool_t *pool)
{
    if (pool->__netbuf)
        free (pool->__netbuf);
}

