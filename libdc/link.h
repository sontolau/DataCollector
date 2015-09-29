#ifndef _DC_LINK_H
#define _DC_LINK_H

#include "libdc.h"

DC_CPP (extern "C" {)

typedef struct _DC_link {
    struct _DC_link *next;
    struct _DC_link *prev;
} DC_link_t;

#define DC_link_init(_link) \
    do {\
        memset (&_link, '\0', sizeof (_link));\
        _link.next = _link.prev = NULL;\
    } while (0)

#define DC_link_init_ring(_link) \
    do {\
        memset (&_link, '\0', sizeof (_link));\
        _link.next = _link.prev = &_link;\
    } while (0)

extern void DC_link_assign (const DC_link_t *link,
                            DC_link_t *newlink);

extern DC_link_t *DC_link_get (DC_link_t *link, int index);

extern void DC_link_insert (DC_link_t *link, DC_link_t *node, int pos);

extern void DC_link_add_behind (DC_link_t *node, DC_link_t *link);
extern void DC_link_add_front (DC_link_t *node, DC_link_t *link);
extern void DC_link_remove (DC_link_t *link);

#define DC_link_add(prev, link) DC_link_add_behind(prev, link)

#define DC_link_container_of(_lkptr, _type, _name) type_container_of(_lkptr, _type, _name)

#define DC_link_foreach(_linkptr, _link, _cond, _flag) \
    for(_linkptr = _link; _cond ; _linkptr = (_flag?_linkptr->next:_linkptr->prev))

#define DC_link_foreach_ring(_linkptr, _link, _cond, _flag) \
    for(_linkptr=_link; _cond; _linkptr = (_flag?_linkptr->next:_linkptr->prev))

DC_CPP (})
#endif
