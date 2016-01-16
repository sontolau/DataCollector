#include "queue.h"

DC_INLINE unsigned long long __get_queue_offset (LLVOID_t *curpos, LLVOID_t *startpos, int size)
{
    unsigned int offset = (unsigned char*)curpos - (unsigned char*)startpos;

    return ((int)(offset / sizeof (LLVOID_t)))%size;
}

DC_INLINE LLVOID_t *__recalc_address (DC_queue_t *queue, LLVOID_t *ptr)
{
    return  (queue->PRI (buffer) + __get_queue_offset (++ptr,
                                         queue->PRI (buffer),
                                         queue->size));
}

int DC_queue_init (DC_queue_t *queue, unsigned int size, LLVOID_t zero)
{
    int i = 0;

    queue->__buffer = (LLVOID_t*)calloc (size, sizeof (LLVOID_t));
    if (!queue->__buffer) {
        return ERR_FAILURE;
    }

    queue->zero_value = zero;
    queue->size       = size;
    for (i=0; i<size; i++) {
        queue->PRI (buffer)[i] = zero;
    }

    queue->PRI (tail_ptr) = queue->PRI (head_ptr) = queue->PRI (buffer);
    return ERR_OK;
}


int DC_queue_is_empty (const DC_queue_t *queue)
{
    return queue->__tail_ptr == queue->__head_ptr &&\
           *queue->__head_ptr== queue->zero_value;
}


int DC_queue_is_full (const DC_queue_t *queue)
{
    return queue->__tail_ptr == queue->__head_ptr &&\
           *queue->__head_ptr != queue->zero_value;
}


int DC_queue_add (DC_queue_t *queue, LLVOID_t obj, int overwrite)
{
    if (!overwrite && DC_queue_is_full (queue)) {
        return ERR_FULL;
    }

    (*queue->PRI (head_ptr)) = obj;
    queue->PRI (head_ptr)  = __recalc_address (queue, queue->PRI (head_ptr));
    return ERR_OK;
}

int DC_queue_get_length (const DC_queue_t *queue)
{
    if (DC_queue_is_empty (queue)) return 0;
    if (DC_queue_is_full (queue)) return queue->size;
    
    if (queue->PRI (head_ptr) > queue->PRI (tail_ptr)) {
        return queue->PRI (head_ptr) - queue->PRI (tail_ptr);
    }

    return queue->size - (queue->PRI (tail_ptr)-queue->PRI (head_ptr));
}

LLVOID_t DC_queue_fetch (DC_queue_t *queue)
{
    LLVOID_t obj;

    if (DC_queue_is_empty (queue)) {
        return queue->zero_value;
    }

    obj = *queue->PRI (tail_ptr);
    (*queue->PRI (tail_ptr)) = queue->zero_value;
    queue->PRI (tail_ptr)  = __recalc_address (queue, queue->PRI (tail_ptr));
    return obj;
}

void DC_queue_clear (DC_queue_t *queue)
{    
    while (DC_queue_fetch (queue) != queue->zero_value);
}

void DC_queue_destroy (DC_queue_t *queue)
{
    DC_queue_clear (queue);
    
    if (queue->PRI (buffer)) {
        free (queue->PRI (buffer));
    }
}
