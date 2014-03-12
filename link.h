#ifndef _DC_LINK_H
#define _DC_LINK_H

#include "libdc.h"

typedef struct link {
    struct link *next;
    struct link *prev;
} DC_link_t;


extern void DC_link_add_after (DC_link_t *before, DC_link_t *link);
extern void DC_link_add_before (DC_link_t *after, DC_link_t *link);
extern void DC_link_remove (DC_link_t *link);


#define DC_link_container_of(_lkptr, _type, _name) \
    ((_type*)(((unsigned long)_lkptr)-(unsigned long)(&(((_type*)0)->_name))))

#define DC_link_foreach(_link, _ptr) \
    for(_link=_ptr; _link!=NULL; _link=_link->next)

#endif
