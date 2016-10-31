#ifndef _DC_LIST_H
#define _DC_LIST_H

#include "thread.h"
#include "link.h"
#include "list.h"

CPP(extern "C" {)

struct _DC_node {
    DC_link_t link;
    OBJ_t data;
};

typedef struct _DC_list 
{
    unsigned int count;
    OBJ_t        zero;
    DC_link_t nodes;
    DC_thread_rwlock_t rwlock;
} DC_list_t;

extern err_t  DC_list_init (DC_list_t *list, 
                          OBJ_t zero);

// append a object at the end of list.
extern err_t DC_list_add (DC_list_t *list, OBJ_t);

// insert a object with a index.
extern err_t DC_list_insert_at_index (DC_list_t *list, OBJ_t, uint32_t index);

// get a object at the index.
extern OBJ_t DC_list_get_at_index (DC_list_t *list, uint32_t index);

// remove a object at the index from list.
extern OBJ_t DC_list_remove_at_index (DC_list_t *list,
                                          uint32_t index);

extern uint32_t DC_list_get_length (DC_list_t *list);
//
extern void DC_list_clean (DC_list_t *list);

// remove a object from list.
extern err_t DC_list_remove (DC_list_t *list, OBJ_t obj);

extern OBJ_t DC_list_next (DC_list_t *list,  void**saveptr);

extern void DC_list_destroy (DC_list_t *list);


CPP(})
#endif
