#include "libdc.h"
#include "thread.h"


DC_task_manager_t manager;
void __task (void *data)
{
    printf ("Thread %u is running.\n", pthread_self ());
}

void func(void*data)
{
    while (1) {
        DC_task_manager_run_task (&manager, __task, NULL, 1);
    }
}


int main ()
{

    DC_thread_t threads[5];

    int i;

    DC_task_manager_init (&manager, 10);

    for (i=0; i<5; i++) {
       DC_thread_init (&threads[i], 0);
       DC_thread_run (&threads[i], func, NULL);
    }

    sleep (10000);
    return 0;
}
