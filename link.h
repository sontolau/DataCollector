#ifndef _DC_LINK_H
#define _DC_LINK_H


DC_CPP (extern "C" {)

typedef struct _DC_link {
    char   class_name[SZ_CLASS_NAME+1];
    struct _DC_link *next;
    struct _DC_link *prev;
} DC_link_t;

#define DC_link_init(_link, _name) \
    do {\
        memset (&_link, '\0', sizeof (_link));\
        _link.next = _link.prev = NULL;\
        if (_name) {\
            strncpy (_link.class_name, (char*)_name, SZ_CLASS_NAME);\
        }\
    } while (0)

#define DC_link_init2(_link, _name) \
    do {\
        memset (&_link, '\0', sizeof (_link));\
        _link.next = _link.prev = &_link;\
        if (_name) {\
            strncpy (_link.class_name, (char*)_name, SZ_CLASS_NAME);\
        }\
    } while (0)

extern void DC_link_assign (const DC_link_t *link,
                            DC_link_t *newlink);


extern void DC_link_add_behind (DC_link_t *node, DC_link_t *link);
extern void DC_link_add_front (DC_link_t *node, DC_link_t *link);
extern void DC_link_remove (DC_link_t *link);

#define DC_link_add(prev, link) DC_link_add_behind(prev, link)

#define DC_link_container_of(_lkptr, _type, _name) type_container_of(_lkptr, _type, _name)

#define DC_link_foreach_forward(_link, _header) \
    for(_link=_header->next; (_link && _link != _header); _link=_link->next)

#define DC_link_foreach_back(_link, _header) \
    for(_link=_header->prev; (_link && _link != _header); _link=_link->prev)

#define DC_link_foreach(_link, _header) DC_link_foreach_forward(_link, _header)

DC_CPP (})
#endif
