#include <stdio.h>
#include "link.h"

void __link_insert (DC_link_t *prev, DC_link_t *link, DC_link_t *next)
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

void __link_remove (DC_link_t *prev, DC_link_t *link, DC_link_t *next)
{
    if (prev) {
        prev->next = next;
    }

    if (next) {
        next->prev = prev;
    }
}

void DC_link_add_after (DC_link_t *before, DC_link_t *link)
{
    __link_insert (before, link, before?before->next:NULL);
}

void DC_link_add_before (DC_link_t *after, DC_link_t *link)
{
    __link_insert (after?after->prev:NULL, link, after);
}

void DC_link_remove (DC_link_t *link)
{
    __link_remove (link?link->prev:NULL, link, link?link->next:NULL);
}
