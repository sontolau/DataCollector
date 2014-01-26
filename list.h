#ifndef _DC_LIST_H
#define _DC_LIST_H

#include "libdc.h"
#include "link.h"


typedef struct DC_list 
{
    unsigned int count;
    DC_link_t __head;
    DC_link_t __tail; 
} DC_list_t;

extern int  DC_list_init (DC_list_t *list,...);
extern void DC_list_add (DC_list_t *list, void *obj);
extern void *DC_list_get_object_at_index (DC_list_t *list, unsigned int index);
extern void DC_list_remove_object_at_index (DC_list_t *list, 
                                            unsigned int index);
extern void DC_list_remove_object (DC_list_t *list, void *obj);
extern void *DC_list_next_object (const DC_list_t *list, void **saveptr);
extern void DC_list_destroy (DC_list_t *list);
#endif
