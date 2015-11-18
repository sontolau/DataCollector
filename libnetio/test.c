#include <libdc/libdc.h>
#include <libdc/thread.h>
static int i = 0;
static int y = 0;

void task_func (void *d1, void *d2)
{
    printf ("%d: %s\n", ++i, (char*)d2);
}

#define HELLO "hello\n"

DC_task_queue_t task_queue;
void main ()
{
#if 0
    #endif
    
    if (DC_task_queue_init (&task_queue, 100, 1, task_func, NULL)) {
        printf ("Can not init task queue.\n");
        exit (1);
    }

    while (1) {
       DC_task_queue_run_task (&task_queue, HELLO, 1);
    }

}
