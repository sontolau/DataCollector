#ifndef _DC_LINK_H
#define _DC_LINK_H

#include "libdc.h"

CPP (extern "C" {)

struct _DC_Link {
    struct _DC_Link *next;
    struct _DC_Link *prev;
}DC_link_t;

#define LINK_INIT (struct _DC_Link){NULL, NULL}


extern void DC_link_add (DC_link_t *node, DC_link_t *link);
extern void DC_link_remove (DC_link_t *link);

#define DC_link_container_of(_ptr, _type, _name) ((_type*)(((unsigned long)_ptr)-(unsigned long)(&(((_type*)0)->_name))))

#define DC_link_foreach(_linkptr, _header) \
    for(_linkptr=_header.next; _linkptr; _linkptr = _linkptr->next)

CPP (})
#endif
