#include "buffer.h"

int DC_buffer_pool_init (DC_buffer_pool_t *pool, int num, unsigned int size)
{
    int i = 0;
    DC_buffer_t *buf_ptr = NULL;
    register unsigned char *ptr = NULL;

    memset (pool, '\0', sizeof (DC_buffer_pool_t));
    pool->unit_size     = size;
    pool->num_total     = num;
    pool->num_left      = num;
    if (!(pool->PRI (buf_ptr) = (DC_buffer_t*)malloc 
                          (num*(size+sizeof(DC_buffer_t))))) {
        return ERR_FAILURE;
    }

    if (DC_queue_init (&pool->PRI (buf_queue), num, 0) != ERR_OK) {
        return ERR_FAILURE;
    }

    ptr = (unsigned char*)pool->PRI (buf_ptr);
    
    for (i=0; i<num; i++) {
        buf_ptr = (DC_buffer_t*)ptr;
        ptr   += (size + sizeof (DC_buffer_t));

        buf_ptr->size   = size;
        buf_ptr->length = 0;
        memset (buf_ptr->buffer, '\0', size);
        DC_queue_add (&pool->PRI (buf_queue), (obj_t)buf_ptr, 0);
    }

    return ERR_OK;
}

DC_buffer_t *DC_buffer_pool_alloc (DC_buffer_pool_t *pool, 
                                   unsigned int size)
{
    if (pool->unit_size < size || DC_queue_is_empty (&pool->PRI (buf_queue))) {
        return NULL;
    }
    pool->num_left--;
    return (DC_buffer_t*)DC_queue_fetch (&pool->PRI (buf_queue));
}


void     DC_buffer_pool_free (DC_buffer_pool_t *pool, DC_buffer_t *buf)
{
    if (buf) {
        buf->length = 0;
        buf->size   = pool->unit_size;

        memset (buf->buffer, '\0', pool->unit_size);

        DC_queue_add (&pool->PRI (buf_queue), (obj_t)buf, 0);
        pool->num_left++;
    }
}

void DC_buffer_pool_destroy (DC_buffer_pool_t *pool)
{
    if (pool->PRI (buf_ptr)) {
        DC_queue_destroy (&pool->PRI (buf_queue));
        free (pool->PRI (buf_ptr));
    }
}

