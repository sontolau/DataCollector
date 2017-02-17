#include "libdc.h"
#include "queue.h"
#include <signal.h>
#include "conf.h"
#include "list.h"

#ifndef OS_WINDOWS
DC_queue_t queue;
unsigned int x = 0;
unsigned int y = 0;
volatile int ext = 0;

void sig(int s) {
    if (s == SIGINT) {
        if (ext == 1) exit(0);
        ext = 1;
        return;
    }

    printf("Writing count: %u Reading count: %u\n", x, y);
    alarm(1);
}

void *print_func(void *data) {
    OBJ_t e;

    while ((e = DC_queue_fetch(&queue, TRUE, 0)) != (OBJ_t) NULL) {
        y++;
        //fprintf (stderr, "Reading: %u\n", e);
    }
}


bool_t print_conf(const char *sec,
                  const char *key,
                  char *value) {
    printf("%s: %s = %s\n", sec, key, value);
    return TRUE;
}

int func()
{
	return 0;
}

void print_list(DC_list_t *list) {
    OBJ_t obj;
    void *data = NULL;

    printf("[");
    while ((obj = DC_list_next(list, &data))) {
        printf("%d,", obj);
    }
    printf("]\n");
}

void main(int argc, char *argv[]) {
	func() ? 1 : 0;

#ifdef QUEUE
    DC_thread_t t;
    time_t tm;

    DC_queue_init (&queue, 100, 0);
    DC_thread_create (&t, print_func, NULL);
    signal (SIGALRM, sig);
    //signal (SIGINT, sig);
    alarm (1);

    usleep (1);
    while (!ext) {
        tm = (time(NULL)/1000);
        //fprintf (stderr, "Writing : %u\n", t);
        if (ISERR (DC_queue_add (&queue, (OBJ_t)tm, TRUE, 0))) {
            fprintf (stderr, "Queue is error\n");
            break;
        }
        x++;
    }

    sleep (100);
#elif defined (CONF)
    exit(DC_read_ini (argv[1], print_conf));
#elif defined (LIST)
    int i = 0;
    DC_list_t list;

    DC_list_init (&list, (OBJ_t)0);

    printf ("Insert 10 numbers (from 1-10):\n");
    for (i=0; i<10; i++) {
        DC_list_add (&list, i+1);
    }

    print_list(&list);

    printf ("Insert 22 at index of 2\n");
    DC_list_insert_at_index (&list, 22, 2);
    print_list (&list);

    printf ("Remove the number at index of 1\n");
    DC_list_remove_at_index (&list, 1);
    print_list (&list);

    printf ("Print the number at index of 8\n");
    printf ("%d\n", (int)DC_list_get_at_index (&list, 8));

    DC_list_clean (&list);
    print_list (&list);

    DC_list_destroy (&list);
#endif
}

#endif
