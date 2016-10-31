#ifndef _DC_LINK_H
#define _DC_LINK_H

#include "libdc.h"

CPP (extern "C" {)

typedef struct _DC_link {
    struct _DC_link *next;
    struct _DC_link *prev;
} DC_link_t;

#define LINK_INIT (struct _DC_Link){NULL, NULL}

#define DC_link_init(link) {link.next = NULL;link.prev=NULL}
#define DC_link_init_ring(link) {link.next = &link; link.prev=&link;}

extern void DC_link_add(__in__ DC_link_t *node, __in__ DC_link_t *link);

extern void DC_link_remove(__in__ DC_link_t *link);

#define DC_link_container_of(_ptr, _type, _name) ((_type*)(((unsigned long)_ptr)-(unsigned long)(&(((_type*)0)->_name))))

#define DC_link_foreach(_linkptr, _header) \
    for(_linkptr=_header.next; _linkptr && _linkptr != &_header; _linkptr = _linkptr->next)

CPP (})
#endif
