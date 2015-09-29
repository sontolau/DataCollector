#include "list.h"

DC_list_elem_t *__find_elem_at_index (DC_list_t *list, 
                                      unsigned int index,
                                      int *flag) {
    register DC_link_t *linkptr = NULL;

    if (index > list->count - 1) {
        return NULL;
    }

    if (index < (int)((list->count - 1)/2)) {
        DC_link_foreach (linkptr,
                         list->PRI (head_ptr).next,
                         linkptr && index,
                         1) index--;
        if (flag) *flag = 1;
    } else {
        index = (list->count - 1) - index;
        DC_link_foreach (linkptr,
                         list->PRI (tail_ptr).prev,
                         linkptr && index,
                         0) index--;
        if (flag) *flag = 0;
    }

    return CONTAINER_OF(linkptr, DC_list_elem_t, PRI (link));
}

int  DC_list_init (DC_list_t *list, 
                   void (*destroy_cb) (DC_list_elem_t*, void *), 
                   void *data, 
                   ...) {
    void *arg = NULL;
    va_list ap;

    memset (list, '\0', sizeof (DC_list_t));
    list->destroy_cb = destroy_cb;
    list->data       = data;

    DC_link_add_behind (&list->PRI (head_ptr), &list->PRI (tail_ptr));

    va_start (ap, data);
    while ((arg = va_arg (ap, void*))) {
        DC_list_add_object (list, arg);
    }
    va_end (ap);

    return ERR_OK;
}

int DC_list_add_object (DC_list_t *list, DC_list_elem_t *obj) {
    DC_link_add_front (&list->PRI (tail_ptr), &obj->PRI (link));
    list->count++;
    return ERR_OK;
}

int DC_list_insert_object_at_index (DC_list_t *list, 
                                    DC_list_elem_t *obj, 
                                    unsigned int index)
{
    DC_list_elem_t *listptr = NULL;

    listptr = __find_elem_at_index (list, index, NULL);
    if (!listptr) return -1;

    DC_link_add_front (&listptr->PRI (link), &obj->PRI (link));
    list->count++;

    return ERR_OK;
}

DC_list_elem_t *DC_list_get_object_at_index (DC_list_t *list, unsigned int index) {
    int flag;
    DC_list_elem_t *obj = NULL;
    DC_link_t *linkptr = NULL;

    obj = __find_elem_at_index (list, index, &flag);
    if (obj) linkptr = &obj->PRI (link);

    return linkptr?CONTAINER_OF (linkptr, DC_list_elem_t, PRI (link)):NULL;
}

DC_list_elem_t *DC_list_remove_object_at_index (DC_list_t *list, unsigned int index) {
    
    DC_list_elem_t *obj = NULL;

    obj = DC_list_get_object_at_index (list, index);
    
    if (obj) {
        DC_list_remove_object (list, obj);
    }

    return obj;
}

void DC_list_remove_object (DC_list_t *list, DC_list_elem_t *obj) 
{
    if (list->destroy_cb) list->destroy_cb (obj, list->data);
    DC_link_remove (&obj->PRI (link));
    list->count--;
}

void DC_list_remove_all_objects (DC_list_t *list)
{
    while (list->count > 0) {
        DC_list_remove_object_at_index (list, 0);
    }
}

void DC_list_destroy (DC_list_t *list) {
    DC_list_remove_all_objects (list);
}

void DC_list_loop (const DC_list_t *list, int (*cb)(DC_list_elem_t*))
{
    DC_link_t *head, *tail;
    DC_link_t *curlist;
    DC_list_elem_t *cr;

    head = (DC_link_t*)&list->PRI (head_ptr);
    tail = (DC_link_t*)&list->PRI (tail_ptr);
    curlist = head->next;

    while (curlist != tail) {
        cr = CONTAINER_OF (curlist, DC_list_elem_t, PRI (link));
        if (cb) {
            if (!cb (cr)) {
                break;
            }
        }
        curlist = curlist->next;
    }
}

DC_list_elem_t *DC_list_next_object (const DC_list_t *list, void **saveptr) 
{
    DC_link_t *cur = *saveptr;
    static DC_link_t *head_ptr,*tail_ptr;

    if (cur == NULL) {
        head_ptr = (DC_link_t*)&list->PRI (head_ptr);
        tail_ptr = (DC_link_t*)&list->PRI (tail_ptr);
        cur = head_ptr->next;
    }

    if (cur == tail_ptr) {
        *saveptr = NULL;
        cur      = NULL;
    } else {
        *saveptr = cur->next;
    }

    return CONTAINER_OF (cur, DC_list_elem_t, PRI (link));
}

/*
DC_list_elem_t **DC_list_to_array (const DC_list_t *list, int *num)
{
    DC_list_elem_t **array = NULL;
    void *saveptr = NULL;
    int  i = 0;
    DC_list_elem_t *obj = NULL;

    *num = 0;
    if (list->count) {
        array = (DC_list_elem_t**)calloc (list->count, sizeof (DC_list_elem_t*));
        if (array) {
            while ((obj = DC_list_next_object (list, &saveptr))) {
                array[i++] = obj;
            }
        }
    }
 
    *num = i;
    return array;
}
*/
#ifdef LIST_DEBUG

struct Node {
    DC_list_elem_t list;
    char name[255];
};

int main () {
    struct Node nodes[10];
    DC_list_elem_t *elem = NULL;
    struct Node *nodeptr;

    void *saveptr = NULL;

    DC_list_t list;
    if (DC_list_init (&list, NULL) < 0) {
        printf ("DC_list_init failed\n");
        exit (1);
    }

    for (i=0; i<10; i++) {
        sprintf (nodes[i].name, "Index %d at %p", i, &nodes[i]);
        DC_list_add_object (&list, &nodes[i]);
    }

    while ((elem = DC_list_next_object (&list, &saveptr))) {
        nodeptr = type_container_of (elem, struct Node, list);
        printf ("name: %s\n", nodeptr->name);
    }

    DC_list_destroy (&list);
    return 0;
}


#endif 
