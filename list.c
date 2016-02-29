#include "list.h"

struct __list_node {
	DC_link_t link;
	LLVOID_t atom;
};

struct __list_node *__find_elem_at_index (DC_list_t *list,
                                      unsigned int index,
                                      int *flag) {
    register DC_link_t *linkptr = NULL;

    if (index > list->count - 1) {
        return NULL;
    }

    if (index < (int)((list->count - 1)/2)) {
        DC_link_foreach (linkptr,
                         list->elements,
                         1) {
        	if (index <= 0) {
        		break;
        	}
        	index--;
        }
        if (flag) *flag = 1;
    } else {
        index = (list->count - 1) - index;
        DC_link_foreach (linkptr,
                         list->elements,
                         0) {
        	if (index <= 0) break;
        	index--;
        }
        if (flag) *flag = 0;
    }

    return DC_link_container_of(linkptr, struct __list_node, link);
}

int  DC_list_init (DC_list_t *list, 
				   LLVOID_t derval,
				   void (*cb) (LLVOID_t, void*),
				   void *data,
                   ...) {
    LLVOID_t arg;
    va_list ap;

    memset (list, '\0', sizeof (DC_list_t));

    DC_link_init (list->elements);
    list->data = data;
    list->remove_cb = cb;
    va_start (ap, data);
    while ((arg = va_arg (ap, LLVOID_t))) {
        DC_list_add (list, arg);
    }
    va_end (ap);

    return ERR_OK;
}

int DC_list_add (DC_list_t *list, LLVOID_t data) {
	struct __list_node *node = NULL;

	if ((node = DC_malloc (sizeof (struct __list_node)))) {
		node->atom = data;
		DC_link_add (list->elements.prev, &node->link);
	}
    list->count++;
    return ERR_OK;
}

int DC_list_insert_at_index (DC_list_t *list,
                             LLVOID_t data,
                             unsigned int index)
{
    struct __list_node *node = NULL;
    struct __list_node *newnode = NULL;

    if ((newnode = DC_malloc (sizeof (struct __list_node)))) {
    	if (!(node = __find_elem_at_index (list, index, NULL))) {
    		DC_free (newnode);
    		return ERR_FAILURE;
    	}
    	newnode->atom = data;
    	DC_link_add (node->link.prev, &newnode->link);
    	list->count++;

    	return ERR_OK;
    }

    return ERR_FAILURE;
}

LLVOID_t DC_list_get_at_index (DC_list_t *list, unsigned int index) {
    struct __list_node *node;

    if (!(node = __find_elem_at_index (list, index, NULL))) {
    	return list->defvalue;
    }

    return node->atom;
}

LLVOID_t DC_list_remove_at_index (DC_list_t *list, unsigned int index) {
    
    struct __list_node *node = NULL;
    LLVOID_t value;

    if ((node = __find_elem_at_index (list, index, NULL))) {
		if (list->remove_cb) {
			list->remove_cb(node->atom, list->data);
		}
    	DC_link_remove (&node->link);
    	list->count--;
    	value = node->atom;
    	DC_free (node);
    	return value;
    }

    return list->defvalue;
}

void DC_list_remove (DC_list_t *list, LLVOID_t data)
{
	register DC_link_t *linkptr;
	struct __list_node *node;

	DC_link_foreach (linkptr, list->elements, 1) {
		node = DC_link_container_of (linkptr, struct __list_node, link);
		if (node->atom == data) {
			if (list->remove_cb) {
				list->remove_cb (node->atom, list->data);
			}
			DC_link_remove (&node->link);
			list->count--;
			DC_free (node);
			break;
		}
	}
}

void DC_list_clean (DC_list_t *list)
{
    while (list->count > 0) {
        DC_list_remove_at_index (list, 0);
    }
}

void DC_list_destroy (DC_list_t *list) {
    DC_list_clean (list);
}

void DC_list_loop (const DC_list_t *list, int (*cb)(LLVOID_t))
{
	register DC_link_t *linkptr;
	struct __list_node *node;

	DC_link_foreach (linkptr, list->elements, 1) {
		node = DC_link_container_of (linkptr, struct __list_node, link);
		if (cb) cb (node->atom);
	}
}

LLVOID_t DC_list_next (const DC_list_t *list, void **saveptr)
{
    register DC_link_t *cur = (DC_link_t*)*saveptr;
    struct __list_node *node = NULL;

   // static DC_link_t *head_ptr = NULL,*tail_ptr = NULL;

    if (cur == NULL) {
        cur = list->elements.next;
    }

    if (cur != &list->elements) {
    	node = DC_link_container_of (cur, struct __list_node, link);
    	*saveptr = cur->next;
    	return node->atom;
    }

    return list->defvalue;
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

void print_list(DC_list_t *list)
{
   printf ("List elements: %d: \n", list->count);
   LLVOID_t elem;
   void *saveptr = NULL;

   while ((elem = DC_list_next (list, &saveptr))) {
        printf ("%d\n", (int)elem);
    } 
}

int main () {
    DC_list_t list;
    LLVOID_t elem;
    int i;

    if (DC_list_init (&list, 0, NULL, NULL, NULL) < 0) {
        printf ("DC_list_init failed\n");
        exit (1);
    }

    for (i=1; i<10; i++) {
        DC_list_add (&list, (LLVOID_t)i);
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
