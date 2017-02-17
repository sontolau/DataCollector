#include "mm.h"
#include "list.h"
#include "errcode.h"

err_t DC_list_init(DC_list_t *list,
                   OBJ_t zero) {
    memset(list, '\0', sizeof(DC_list_t));
    DC_link_init_ring (list->nodes);
    list->count = 0;
    if (ISERR(DC_thread_rwlock_init(&list->rwlock))) {
        return E_ERROR;
    }

    return E_OK;
}

err_t DC_list_add(DC_list_t *list, OBJ_t data) {
    struct _DC_node *node = NULL;

    DC_thread_rwlock_wrlock(&list->rwlock);

    if ((node = DC_malloc (sizeof(struct _DC_node)))) {
        node->data = data;
        DC_link_add(list->nodes.prev, &node->link);
    }

    list->count++;
    DC_thread_rwlock_unlock(&list->rwlock);

    return E_OK;
}

err_t DC_list_insert_at_index(DC_list_t *list,
                              OBJ_t data,
                              uint32_t index) {
    struct _DC_node *newnode = NULL;
    DC_link_t *linkptr = NULL;

    DC_thread_rwlock_wrlock(&list->rwlock);

    if (index >= list->count) {
        DC_set_errcode (E_OUTOFBOUND);
        DC_thread_rwlock_unlock(&list->rwlock);
        return E_ERROR;
    }

    if ((newnode = DC_malloc (sizeof(struct _DC_node)))) {
        newnode->data = data;

        DC_link_foreach (linkptr, list->nodes) {
            if (index <= 0) break;
            index--;
        }

        DC_link_add(linkptr->prev, &newnode->link);
        list->count++;
    }

    DC_thread_rwlock_unlock(&list->rwlock);
    return E_OK;
}

OBJ_t DC_list_get_at_index(DC_list_t *list, uint32_t index) {
    struct _DC_node *node;
    DC_link_t *ptr;

    DC_thread_rwlock_rdlock(&list->rwlock);

    if (index >= list->count) {
        DC_set_errcode (E_OUTOFBOUND);
        DC_thread_rwlock_unlock(&list->rwlock);
        return list->zero;
    }

    DC_link_foreach (ptr, list->nodes) {
        if (index <= 0) break;
        index--;
    }

    node = DC_link_container_of (ptr, struct _DC_node, link);
    DC_thread_rwlock_unlock(&list->rwlock);

    return node->data;
}

OBJ_t DC_list_remove_at_index(DC_list_t *list, uint32_t index) {

    struct _DC_node *node = NULL;
    OBJ_t value;
    DC_link_t *ptr = NULL;

    DC_thread_rwlock_wrlock(&list->rwlock);

    if (index >= list->count) {
        DC_set_errcode (E_OUTOFBOUND);
        DC_thread_rwlock_unlock(&list->rwlock);
        return list->zero;
    }

    DC_link_foreach (ptr, list->nodes) {
        if (index <= 0) break;
        index--;
    }

    node = DC_link_container_of (ptr, struct _DC_node, link);
    DC_link_remove(ptr);
    list->count--;
    value = node->data;
    DC_free (node);
    DC_thread_rwlock_unlock(&list->rwlock);

    return value;
}

err_t DC_list_remove(DC_list_t *list, OBJ_t data) {
    DC_link_t *ptr;
    struct _DC_node *node;

    DC_thread_rwlock_wrlock(&list->rwlock);

    DC_link_foreach (ptr, list->nodes) {
        node = DC_link_container_of (ptr, struct _DC_node, link);
        if (node->data == data) {
            DC_link_remove(ptr);
            list->count--;
            DC_free (node);
            break;
        }
    }

    DC_thread_rwlock_unlock(&list->rwlock);
    return E_OK;
}

uint32_t DC_list_get_length(DC_list_t *list) {
    uint32_t c = 0;

    DC_thread_rwlock_rdlock(&list->rwlock);

    c = list->count;
    DC_thread_rwlock_unlock(&list->rwlock);

    return c;
}


void DC_list_clean(DC_list_t *list) {
    while (DC_list_get_length(list) > 0) {
        DC_list_remove_at_index(list, 0);
    }
}

void DC_list_destroy(DC_list_t *list) {
    DC_list_clean(list);
    DC_thread_rwlock_destroy(&list->rwlock);
}


OBJ_t DC_list_next(DC_list_t *list, void **saveptr) {
    register DC_link_t *cur = (DC_link_t *) *saveptr;
    struct _DC_node *node = NULL;
    OBJ_t obj = list->zero;

    DC_thread_rwlock_rdlock(&list->rwlock);

    if (cur == NULL) {
        cur = list->nodes.next;
    }

    if (cur != &list->nodes) {
        node = DC_link_container_of (cur, struct _DC_node, link);
        *saveptr = cur->next;
        obj = node->data;

    }

    DC_thread_rwlock_unlock(&list->rwlock);
    return obj;
}

/*
*/
#ifdef LIST_DEBUG

void print_list(DC_list_t *list)
{
   printf ("List nodes: %d: \n", list->count);
   OBJ_t elem;
   void *saveptr = NULL;

   while ((elem = DC_list_next (list, &saveptr))) {
        printf ("%d\n", (int)elem);
    } 
}

int main () {
    DC_list_t list;
    OBJ_t elem;
    int i;

    if (DC_list_init (&list, 0, NULL, NULL, NULL) < 0) {
        printf ("DC_list_init failed\n");
        exit (1);
    }

    for (i=1; i<10; i++) {
        DC_list_add (&list, (OBJ_t)i);
    }
    print_list (&list);

    printf ("Remove at index: 0\n");
    DC_list_remove_at_index (&list, 0);
    print_list (&list);

    printf ("Remove last element:\n");
    DC_list_remove_at_index (&list, list.count-1);
    print_list (&list);

    printf ("Add object: 10\n");
    DC_list_add (&list, 10);
    print_list (&list);

    printf ("Insert object at index: 1\n");
    DC_list_insert_at_index (&list, 11, 1);
    print_list (&list);

    printf ("The 4th object is: %d", DC_list_get_at_index (&list, 4));
    print_list (&list);
    DC_list_destroy (&list);
    return 0;
}


#endif 
