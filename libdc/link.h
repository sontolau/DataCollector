#ifndef _DC_LINK_H
#define _DC_LINK_H

#include "libdc.h"

DC_CPP (extern "C" {)

typedef struct _DC_link {
    struct _DC_link *next;
    struct _DC_link *prev;
} DC_link_t;


#define DC_link_init(_link) (_link.next = _link.prev = &_link)
#define DC_LINK_DEFINE(_link) \
	DC_link_t _link = {\
        .next = &_link,\
        .prev = &_link\
    };

extern void DC_link_add (DC_link_t *node, DC_link_t *link);
extern void DC_link_remove (DC_link_t *link);

#define DC_link_container_of(_lkptr, _type, _name) type_container_of(_lkptr, _type, _name)

#define DC_link_foreach(_linkptr, _header, _flag) \
    for(_linkptr=(_flag?_header.next:_header.prev); _linkptr != &_header; _linkptr = (_flag?_linkptr->next:_linkptr->prev))

DC_CPP (})
#endif
