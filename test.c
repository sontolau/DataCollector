#include "stdio.h"
#include "link.h"
#include "list.h"
#include "dict.h"
#include "queue.h"
#include "signal.h"
#include "buffer.h"

struct x {
    int y;
    void *p;
    DC_link_t link;
};

void func_test (int *x, char *y, float *m, void *n)
{
    printf ("%lx,%lx,%lx,%lx\n", &x,&y,&m,&n);
}

void __sig_func (DC_sig_t sig, void *dat)
{
    printf ("Signal %s\n", sig==1?"A":"B");
}


int main ()
{
    int i= 0;
    DC_buffer_pool_t  pool;
    DC_buffer_t  *bufptr;

    if (DC_buffer_pool_init (&pool, 10, 100)) {
        fprintf (stderr, "DC_buffer_pool_init failed\n");
        return -1;
    }
    int j = 5;

    for (j=0; j<5; j++) {
        for (i=0; i<100; i++) {
            if ((bufptr = DC_buffer_pool_alloc (&pool, 100))) {
            memset (bufptr->data, '\0', bufptr->length);
            memset (bufptr->data, 'f', 99);
            printf ("data:%s\n", bufptr->data);
            printf ("%p\n", bufptr);
            printf ("%p\n", DC_buffer_from (bufptr->data));
            DC_buffer_pool_free (&pool, DC_buffer_from (bufptr->data));
            }
        }
    }
    DC_buffer_pool_destroy (&pool);
}

int __main ()
{
    HDC  *sig;
    srand (time (NULL));
    sig = DC_signal_alloc ();
    DC_signal_wait_asyn (sig, 0, __sig_func, NULL);
    DC_signal_wait_asyn (sig, 0, __sig_func, NULL);
    int siga[] = {1,2};

    while (1) {
        DC_signal_send (sig, siga[random()%2]);
        usleep (10000);
    }

    return 0;
}

int xxxmain(){
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
