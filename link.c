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

void DC_link_add_behind (DC_link_t *node, DC_link_t *link)
{
    __link_insert (node, link, node?node->next:NULL);
}

void DC_link_add_front (DC_link_t *node, DC_link_t *link)
{
    __link_insert (node?node->prev:NULL, link, node);
}

void DC_link_remove (DC_link_t *link)
{
    __link_remove (link?link->prev:NULL, link, link?link->next:NULL);
}

void DC_link_assign (const DC_link_t *link,
                     DC_link_t *newlink)
{
    DC_link_t *nl = NULL;
    DC_link_t *pl = NULL;

    DC_link_init ((*newlink));
    nl = link->next;
    pl = link->prev;

    if (nl && nl != link) {
        newlink->next = nl;
        nl->prev      = newlink;
    }

    if (pl && pl != link) {
        newlink->prev = pl;
        pl->next      = newlink;
    }
}
