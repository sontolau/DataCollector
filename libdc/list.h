#ifndef _DC_LIST_H
#define _DC_LIST_H

#include "libdc.h"
#include "link.h"

DC_CPP(extern "C" {)

typedef struct _DC_list_elem {
    DC_link_t PRI (link);
} DC_list_elem_t;

typedef struct _DC_list 
{
    unsigned int count;
    void (*destroy_cb) (DC_list_elem_t*, void*);
    void *data;

    DC_link_t PRI (head_ptr);
    DC_link_t PRI (tail_ptr); 
} DC_list_t;

/*
 * initialize a variable of list type, which must be
 * ended with a NULL value.
 * example:
 *    DC_list_t list/
 *    char *name = "John";
 *    DC_list_init (&list, (void*)name, NULL);
 */
extern int  DC_list_init (DC_list_t *list, 
                          void (*destroy_cb) (DC_list_elem_t*, void *data), 
                          void *data,...);

// append a object at the end of list.
extern int DC_list_add_object (DC_list_t *list, DC_list_elem_t *obj);

// insert a object with a index.
extern int DC_list_insert_object_at_index (DC_list_t *list, DC_list_elem_t *obj, unsigned int index);

// get a object at the index.
extern DC_list_elem_t *DC_list_get_object_at_index (DC_list_t *list, unsigned int index);

// remove a object at the index from list.
extern DC_list_elem_t *DC_list_remove_object_at_index (DC_list_t *list, 
                                            unsigned int index);

//
extern void DC_list_remove_all_objects (DC_list_t *list);

// remove a object from list.
extern void DC_list_remove_object (DC_list_t *list, DC_list_elem_t *obj);

extern DC_list_elem_t *DC_list_next_object (const DC_list_t *list, void **saveptr);

extern void DC_list_loop (const DC_list_t *list, int (*cb)(DC_list_elem_t*));

extern void DC_list_destroy (DC_list_t *list);


DC_CPP(})
#endif
