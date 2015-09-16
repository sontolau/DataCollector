#include "queue.h"

DC_INLINE int __get_queue_offset (obj_t *curpos, obj_t *startpos, int size)
{
    unsigned int offset = (unsigned char*)curpos - (unsigned char*)startpos;

    return ((int)(offset / sizeof (obj_t)))%size;
}

int DC_queue_init (DC_queue_t *queue, unsigned int queue_size)
{
    int i = 0;

    queue->__queue_buffer = (obj_t*)calloc (queue_size, sizeof (obj_t));
    if (!queue->__queue_buffer) {
        return ERR_FAILURE;
    }

    queue->queue_size       = queue_size;
    queue->length           = 0;

    for (i=0; i<queue_size; i++) {
        queue->PRI (queue_buffer)[i] = QZERO;
    }

    queue->PRI (tail_ptr) = queue->PRI (head_ptr) = queue->PRI (queue_buffer);
    return ERR_OK;
}


int DC_queue_is_empty (const DC_queue_t *queue)
{
    return queue->__tail_ptr == queue->__head_ptr &&\
           *queue->__head_ptr== QZERO;
}


int DC_queue_is_full (const DC_queue_t *queue)
{
    return queue->__tail_ptr == queue->__head_ptr &&\
           *queue->__head_ptr != QZERO;
}


int DC_queue_add (DC_queue_t *queue, obj_t obj, int overwrite)
{
    if (!overwrite && DC_queue_is_full (queue)) {
        return ERR_FULL;
    }

    *queue->PRI (head_ptr) = obj;
    queue->PRI (head_ptr)  = (obj_t*)(queue->PRI (queue_buffer) + \
                         __get_queue_offset (++queue->PRI (head_ptr), \
                                             queue->PRI (queue_buffer), \
                                             queue->queue_size));
    queue->length = (++queue->length>queue->queue_size?queue->queue_size:queue->length);
    return ERR_OK;
}

obj_t DC_queue_fetch (DC_queue_t *queue)
{
    obj_t obj;

    if (DC_queue_is_empty (queue)) {
        return QZERO;
    }

    obj = *queue->PRI (tail_ptr);
    *queue->PRI (tail_ptr) = QZERO;
    queue->PRI (tail_ptr)  = (obj_t*)(queue->PRI (queue_buffer) + \
                         __get_queue_offset (++queue->PRI (tail_ptr), \
                                             queue->PRI (queue_buffer), \
                                             queue->queue_size));
    queue->length--;
    return obj;
}

void DC_queue_clear (DC_queue_t *queue)
{    
    while (DC_queue_fetch (queue));
}

void DC_queue_destroy (DC_queue_t *queue)
{
    DC_queue_clear (queue);
    
    if (queue->PRI (queue_buffer)) {
        free (queue->PRI (queue_buffer));
    }
}
