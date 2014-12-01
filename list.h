#ifndef _DC_LIST_H
#define _DC_LIST_H

#include "libdc.h"
#include "link.h"

DC_CPP(extern "C" {)

typedef struct _DC_list 
{
    unsigned int count;
    DC_link_t __head;
    DC_link_t __tail; 
} DC_list_t;

/*
 * initialize a variable of list type, which must be
 * ended with a NULL value.
 * example:
 *    DC_list_t list/
 *    char *name = "John";
 *    DC_list_init (&list, (void*)name, NULL);
 */
extern int  DC_list_init (DC_list_t *list,...);

// append a object at the end of list.
extern void DC_list_add_object (DC_list_t *list, void *obj);

// get a object at the index.
extern void *DC_list_get_object_at_index (DC_list_t *list, unsigned int index);

// remove a object at the index from list.
extern void *DC_list_remove_object_at_index (DC_list_t *list, 
                                             unsigned int index);

// remove a object from list.
extern void DC_list_remove_object (DC_list_t *list, void *obj);

// to use this to get all objects in order.
extern void *DC_list_next_object (const DC_list_t *list, void **saveptr);

extern void DC_list_loop (const DC_list_t *list, int (*cb)(void*));

extern void DC_list_destroy (DC_list_t *list,
                             void (*cb)(void*));

DC_CPP(})
#endif
