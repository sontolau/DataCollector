#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "list.h"

struct list_carrier {
    void *obj;
    DC_link_t  link;
};

struct list_carrier *__find_carrier_by_obj (DC_list_t *list, void *obj) {
    register DC_link_t *ptr;
    struct list_carrier *lc;

    ptr = list->__head.next;
    while (ptr && ptr != &list->__tail) {
        lc = DC_link_container_of (ptr, struct list_carrier, link);
        if (lc->obj == obj) {
            return lc;
        }

        ptr = ptr->next;
    }

    return NULL;
}

struct list_carrier *__find_carrier_by_idx (DC_list_t *list, 
                                            unsigned int index) {
    register DC_link_t *ptr;
    struct list_carrier *cr;
    unsigned int loop_times;

    if (index >= list->count) {
        return NULL;
    }

    if (index > (unsigned int)(list->count/2)) {
        ptr = &list->__tail;
        loop_times = list->count - index;
        while (loop_times--) ptr = ptr->prev;
    } else {
        ptr = &list->__head;
        loop_times = index + 1;
        while (loop_times--) ptr = ptr->next;
    }

    cr = DC_link_container_of (ptr, struct list_carrier, link);

    return cr;
}

int  DC_list_init (DC_list_t *list, ...) {
    void *arg = NULL;
    va_list ap;

    memset (list, '\0', sizeof (DC_list_t));
    DC_link_add_after (&list->__head, &list->__tail);
    va_start (ap, list);
    while ((arg = va_arg (ap, void*))) {
        DC_list_add (list, arg);
    }
    va_end (ap);

    return 0;
}

void DC_list_add (DC_list_t *list, void *obj) {
    struct list_carrier *cr;

    cr = (struct list_carrier*)malloc(sizeof (struct list_carrier));
    list->count++;
    
    cr->obj = obj;
    DC_link_add_before (&list->__tail, &cr->link);
}

void *DC_list_get_object_at_index (DC_list_t *list, unsigned int index) {
    struct list_carrier *cr;

    cr = __find_carrier_by_idx (list, index);
    return cr?cr->obj:NULL;
}

void DC_list_remove_object_at_index (DC_list_t *list, unsigned int index) {
    struct list_carrier *cr;

    cr = __find_carrier_by_idx (list, index);
    if (cr) {
        DC_link_remove (&cr->link);
        list->count--;
        free (cr);
    }
}

void DC_list_remove_object (DC_list_t *list, void *obj) {
    struct list_carrier *cr;

    cr = __find_carrier_by_obj (list, obj);
    if (cr) {
        DC_link_remove (&cr->link);
        list->count--;
         free (cr);
    }
}

void *DC_list_next_object (const DC_list_t *list, void **saveptr) {
    DC_link_t *cur = *saveptr;
    static DC_link_t *__head,*__tail;
    struct list_carrier *cr;

    if (cur == NULL) {
        __head = (DC_link_t*)&list->__head;
        __tail = (DC_link_t*)&list->__tail;
        cur = __head->next;
    }

    if (cur == __tail) {
        *saveptr = NULL;
        cur      = NULL;
    } else {
        *saveptr = cur->next;
    }

    cr = (cur?DC_link_container_of (cur, struct list_carrier, link):NULL);
    return cr?cr->obj:NULL;
}

void DC_list_destroy (DC_list_t *list) {
    while (list->count > 0) {
        DC_list_remove_object_at_index (list, list->count-1);
    }
}
