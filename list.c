#include "libdc.h"

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

int  DC_list_init (DC_list_t *list, void (*cb)(void*), ...) {
    void *arg = NULL;
    va_list ap;

    memset (list, '\0', sizeof (DC_list_t));
    DC_link_add_after (&list->__head, &list->__tail);
    list->__destroy_cb = cb;

    va_start (ap, cb);
    while ((arg = va_arg (ap, void*))) {
        DC_list_add_object (list, arg);
    }
    va_end (ap);

    return ERR_OK;
}

int DC_list_add_object (DC_list_t *list, void *obj) {
    struct list_carrier *cr;

    cr = (struct list_carrier*)malloc(sizeof (struct list_carrier));
    if (cr == NULL) {
        return ERR_SYSTEM;
    }

    list->count++;
    
    cr->obj = obj;
    DC_link_add_before (&list->__tail, &cr->link);
    return ERR_OK;
}

int DC_list_insert_object_at_index (DC_list_t *list, void *obj, unsigned int index)
{
    struct list_carrier *cr;
    DC_link_t *linkptr = &list->__head;

    cr = (struct list_carrier*)malloc (sizeof (struct list_carrier));
    if (cr == NULL) {
        return ERR_SYSTEM;
    }

    DC_link_foreach (linkptr, ((DC_link_t*)&list->__head)) {
        if (!index) {
            break;
        }
        index--;
    }

    if (index > 0) {
        return ERR_NOTFOUND;
    }

    cr->obj = obj;
    DC_link_add (linkptr, &cr->link);

    return ERR_OK;
}

void *DC_list_get_object_at_index (DC_list_t *list, unsigned int index) {
    struct list_carrier *cr;

    cr = __find_carrier_by_idx (list, index);
    return cr?cr->obj:NULL;
}

void DC_list_remove_object_at_index (DC_list_t *list, unsigned int index) {
    struct list_carrier *cr;
    void                *obj;

    cr = __find_carrier_by_idx (list, index);
    if (cr) {
        DC_link_remove (&cr->link);
        obj = cr->obj;
        if (list->__destroy_cb) {
            list->__destroy_cb (obj);
        }
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

void DC_list_remove_all_objects (DC_list_t *list)
{
    while (list->count) {
        DC_list_remove_object_at_index (list, 0);
    }
}

void DC_list_destroy (DC_list_t *list) {
    DC_list_remove_all_objects (list);
}

void DC_list_loop (const DC_list_t *list, int (*cb)(void*))
{
    DC_link_t *head, *tail;
    DC_link_t *curlist;
    struct list_carrier *cr;

    head = (DC_link_t*)&list->__head;
    tail = (DC_link_t*)&list->__tail;
    curlist = head->next;

    while (curlist != tail) {
        cr = DC_link_container_of (curlist, struct list_carrier, link);
        if (cr && cb) {
            if (!cb (cr->obj)) {
                break;
            }
        }
        curlist = curlist->next;
    }
}

void *DC_list_next_object (const DC_list_t *list, void **saveptr) 
{
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

void **DC_list_to_array (const DC_list_t *list, int *num)
{
    void **array = NULL;
    void *saveptr = NULL;
    int  i = 0;
    void *obj = NULL;

    *num = 0;
    if (list->count) {
        array = (void**)calloc (list->count, sizeof (void*));
        if (array) {
            while ((obj = DC_list_next_object (list, &saveptr))) {
                array[i++] = obj;
            }
        }
    }
 
    *num = i;
    return array;
}
