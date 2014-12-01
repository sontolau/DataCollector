#include "stdio.h"
#include "link.h"
#include "list.h"
#include "dict.h"
#include "queue.h"

struct x {
    int y;
    void *p;
    DC_link_t link;
};

void func_test (int *x, char *y, float *m, void *n)
{
    printf ("%lx,%lx,%lx,%lx\n", &x,&y,&m,&n);
}
int main(){
#if 0
    struct x q,w,e;

    DC_link_t head = {NULL, NULL};

    q.y=1;
    w.y=2;
    e.y=3;

    DC_link_add_after(&head, &q.link);
    DC_link_add_after(&q.link, &w.link);
    DC_link_add_after(&w.link, &e.link);

    DC_link_t *var;

    DC_link_foreach (var, head.next) {
        struct x *m = DC_link_container_of(var, struct x, link);
        printf ("%d\n", m->y);
    }

#endif
    DC_list_t   list;

    DC_list_init (&list, NULL, (void*)1,(char*)2,(int*)3,(long*)4,(short*)5, NULL);
    printf ("The count of elements: %d\n", list.count);

    DC_list_add_object (&list, (void*)8);
    printf ("The count of elements: %d\n", list.count);

    void *saveptr = NULL;

    void *p;
    while ((p = DC_list_next_object (&list, &saveptr))) {
        printf ("%d\n", (int)p);
    }

    printf ("The object at index 0 is: %d\n", (int)DC_list_get_object_at_index (&list, 0));

    DC_list_remove_object (&list, 5);
    DC_list_destroy (&list);
    saveptr = NULL;
    while ((p = DC_list_next_object (&list, &saveptr))) {
        printf ("%d\n", (int)p);
    }

    DC_list_destroy (&list);
/*
    DC_dict_t  mydict;

    if (DC_dict_init (&mydict, "name", "sonto", "age", "21", NULL) < 0) {
        printf ("DC_dict_init failed.\n");
        exit (1);
    }    

    printf ("%s,%s\n", DC_dict_get_object_with_key (&mydict, "name"), DC_dict_get_object_with_key (&mydict, "age"));
    DC_dict_add_object_with_key (&mydict, "year", DC_NUM_INTEGER(45));

    printf ("year: %d\n", ((DC_number_t*)DC_dict_get_object_with_key (&mydict, "year"))->int_value);

    DC_dict_destroy (&mydict);
*/

    DC_queue_t queue;

    if (DC_queue_init (&queue, 10, 0) < 0) {
        printf ("DC_queue_init failed\n");
        return -1;
    }
    int i;

    for (i=1; i<20; i++) {
       if (DC_queue_push (&queue, (qobject_t)i, 0) == -1) {
            printf ("The queue is full\n");
            break;
        }
    }

    for (i=0; i<12; i++) {
        qobject_t o = DC_queue_pop (&queue);
        if (o == 0) {
            printf ("Queue is empty\n");
            break;
        } else {
            printf ("%u\n", o);
        }
    }

    DC_queue_destroy (&queue);
    return 0; 

}
