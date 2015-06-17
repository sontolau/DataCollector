#include "libdc.h"

int DC_buffer_pool_init (DC_buffer_pool_t *pool, int num, unsigned int size)
{
    int i = 0;
    DC_buffer_t *bufptr = NULL;
    register unsigned char *ptr = NULL;

    memset (pool, '\0', sizeof (DC_buffer_pool_t));

    pool->numbufs       = num;
    pool->num_allocated = 0;
    pool->unit_size     = size;
    if (!(pool->__bufptr= (DC_buffer_t*)malloc 
                          (num*(size+sizeof(DC_buffer_t))))) {
        return ERR_FAILURE;
    }

    ptr = (unsigned char*)pool->__bufptr;
    
    for (i=0; i<num; i++) {
        bufptr = (DC_buffer_t*)ptr;
        ptr   += (size + sizeof (DC_buffer_t));

        bufptr->size   = size;
        bufptr->length = 0;
        bufptr->busy   = 0;
        DC_link_add (&pool->__free_link, &bufptr->__link);
        memset (bufptr->data, '\0', size);
    }

    return ERR_OK;
}

DC_buffer_t *DC_buffer_pool_alloc (DC_buffer_pool_t *pool, 
                                   unsigned int size)
{
    DC_link_t *linkptr = NULL;
    DC_buffer_t  *buffer  = NULL;

    if (pool->unit_size < size) {
        return NULL;
    }

    linkptr = pool->__free_link.next;
    if (linkptr && linkptr != &pool->__free_link) {
        DC_link_remove (linkptr);
        DC_link_add (&pool->__engaged_link, linkptr);
   
        buffer = DC_link_container_of (linkptr, DC_buffer_t, __link);
        buffer->busy = 1;
        pool->num_allocated++;
    }

    return buffer;
}


void     DC_buffer_pool_free (DC_buffer_pool_t *pool, DC_buffer_t *buf)
{
    if (buf) {
        buf->length = 0;
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
    if (pool->__bufptr) {
        free (pool->__bufptr);
    }
}

