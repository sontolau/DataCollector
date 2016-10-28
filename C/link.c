#include "link.h"

INLINE void __link_insert (DC_link_t *prev, DC_link_t *link, DC_link_t *next)
{
    if (link) {
        link->next = next;
        link->prev = prev;
    }

    if (prev) {
        prev->next = link;
    }

    if (next) {
        next->prev = link;
    }
}

INLINE void __link_remove (DC_link_t *prev, DC_link_t *link, DC_link_t *next)
{
    if (prev) {
        prev->next = next;
    }

    if (next) {
        next->prev = prev;
    }
}

void DC_link_add (DC_link_t *node, DC_link_t *link)
{
    __link_insert (node, link, node?node->next:NULL);
}


void DC_link_remove (DC_link_t *link)
{
    __link_remove (link?link->prev:NULL, link, link?link->next:NULL);
}

