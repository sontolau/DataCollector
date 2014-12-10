#include "buffer.h"

int DC_buffer_pool_init (DC_buffer_pool_t *pool, int num, unsigned int size)
{
    int i = 0;
    register DC_buffer_t *bufptr = NULL;

    memset (pool, '\0', sizeof (DC_buffer_pool_t));

    pool->num_allocated = 0;
    pool->__bufptr = (DC_buffer_t*)calloc (num, size + sizeof (DC_buffer_t));
    if (pool->__bufptr == NULL) {
        return -1;
    }

    bufptr = pool->__bufptr;
    for (i=0; i<num; i++) {
        bufptr->id     = (long long)pool->__bufptr+i;
        bufptr->size   = size;
        bufptr->length = 0;
        bufptr->busy   = 0;
        DC_link_add (&pool->__free_link, &bufptr->__link);
        bufptr = (DC_buffer_t*)((unsigned char*)bufptr + (size + sizeof (DC_buffer_t)));
    }

    return 0;
}

DC_buffer_t *DC_buffer_pool_alloc (DC_buffer_pool_t *pool)
{
    DC_link_t *linkptr = NULL;
    DC_buffer_t  *buffer  = NULL;

    linkptr = pool->__free_link.next;
    if (linkptr) {
        DC_link_remove (linkptr);
        DC_link_add (&pool->__engaged_link, linkptr);
   
        buffer = DC_link_container_of (linkptr, DC_buffer_t, __link);
        buffer->busy = 1;
        pool->num_allocated++;
    }

    return buffer;
}

DC_buffer_t *DC_buffer_pool_get (DC_buffer_pool_t *pool, long long id)
{
    long long offset = id - (long long)pool->__bufptr;

    if (offset >= pool->numbufs || offset < 0) {
        return NULL;
    }

    return (DC_buffer_t*)((long long)pool->__bufptr+offset * pool->bufsize);
}

void     DC_buffer_pool_free (DC_buffer_pool_t *pool, DC_buffer_t *buf)
{
    if (buf) {
        buf->length = 0;
        buf->size   = 0;
        buf->busy   = 0;
        DC_link_remove (&buf->__link);
        DC_link_add    (&pool->__free_link, &buf->__link);
        pool->num_allocated--;
    }
}

float    DC_buffer_pool_get_usage (const DC_buffer_pool_t *pool)
{
    return pool->num_allocated / pool->numbufs;
}

void DC_buffer_pool_destroy (DC_buffer_pool_t *pool)
{
    if (pool->__bufptr)
        free (pool->__bufptr);
}

