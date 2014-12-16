#ifndef _DC_LINK_H
#define _DC_LINK_H

#include "libdc.h"

DC_CPP (extern "C" {)

typedef struct _DC_link {
    char   class_name[SZ_CLASS_NAME];
    struct _DC_link *next;
    struct _DC_link *prev;
} DC_link_t;


extern void DC_link_add_after (DC_link_t *before, DC_link_t *link);
extern void DC_link_add_before (DC_link_t *after, DC_link_t *link);
extern void DC_link_remove (DC_link_t *link);

//set a class name for a link.
#define DC_link_set_class_name(link, name)   {strncpy (link->class_name, name, sizeof (link->class_name)-1);}

#define DC_link_add(prev, link) DC_link_add_after(prev, link)

#define DC_link_container_of(_lkptr, _type, _name) \
    ((_type*)(((unsigned long)_lkptr)-(unsigned long)(&(((_type*)0)->_name))))

#define DC_link_foreach_forward(_link, _header) \
    for(_link=_header->next; (_link && _link != _header); _link=_link->next)

#define DC_link_foreach_back(_link, _header) \
    for(_link=_header->prev; (_link && _link != _header); _link=_link->prev)

#define DC_link_foreach(_link, _header) DC_link_foreach_forward(_link, _header)

DC_CPP (})
#endif
