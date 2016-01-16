#ifndef _DC_LIST_H
#define _DC_LIST_H

#include "libdc.h"
#include "link.h"

DC_CPP(extern "C" {)

typedef struct _DC_list 
{
    unsigned int count;
    LLVOID_t defvalue;
    void (*remove_cb) (LLVOID_t, void*);
    void *data;
    DC_link_t elements;
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
                          LLVOID_t defvalue,
                          void (*cb)(LLVOID_t, void*),
                          void *data,
                          ...);

// append a object at the end of list.
extern int DC_list_add (DC_list_t *list, LLVOID_t);

// insert a object with a index.
extern int DC_list_insert_at_index (DC_list_t *list, LLVOID_t, unsigned int index);

// get a object at the index.
extern LLVOID_t DC_list_get_at_index (DC_list_t *list, unsigned int index);

// remove a object at the index from list.
extern LLVOID_t DC_list_remove_at_index (DC_list_t *list,
                                          unsigned int index);

//
extern void DC_list_clean (DC_list_t *list);

// remove a object from list.
extern void DC_list_remove (DC_list_t *list, LLVOID_t obj);

extern LLVOID_t DC_list_next (const DC_list_t *list,  void**saveptr);

extern void DC_list_loop (const DC_list_t *list, int (*cb)(LLVOID_t));

extern void DC_list_destroy (DC_list_t *list);


DC_CPP(})
#endif
