#include "libdc.h"
#include "thread.h"

DC_task_manager_t manager;
static unsigned int count = 0;
DC_task_queue_t   task_queue;

void func (void *data)
{
    printf ("%u thread is running.%u\n", pthread_self (), ++count);
}

#define HELLO "hello"
void thread (void *data)
{
    while (1) {
        //DC_task_manager_run_task (&manager, func, NULL, 1);
        DC_task_queue_run_task (&task_queue, HELLO, 1);
    }
}


void task (void *d1, void *d2)
{
    printf ("%u thread prints %s\n", pthread_self (), (char*)d2);
}

void main ()
{
    DC_thread_t threads[10];

    int i;

    DC_task_queue_init (&task_queue, 100, 5, task ,NULL);
    //DC_task_manager_init (&manager, 5);
    for (i=0; i<10; i++) {
        DC_thread_init (&threads[i], 0);
        DC_thread_run (&threads[i], thread, NULL);
    }

    sleep (10000);
}
