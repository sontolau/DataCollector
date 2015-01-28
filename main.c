#include "notifier.h"
#include "thread.h"

static DC_notifier_t g_notif;
void thread_func (DC_thread_t *th, void*data)
//void func (void *data)
{
    int i= 10;
    //DC_notifier_wait (&g_notif, 0);
    while (i) {
        printf ("Thread %u\n", pthread_self ());
        usleep (1000000);
        i--;
    }
}

void task_func (DC_task_t *task, void *data)
{
    thread_func (NULL, NULL);
}

void thread_status (DC_thread_t *th, void *data, int status)
{
    const char *statusstr[] = {
        "IDLE",
        "RUNNING",
        "TIMEOUT",
        "EXITING",
        "EXITED"
    };

    printf ("Thread status: %s\n", statusstr[status]);
}
int main ()
{
   //DC_notifier_t notif;
   int i=0;
//   DC_thread_t   thread;
   //pthread_t thread;
   DC_thread_pool_manager_t pool_manager;

   //DC_notifier_init (&g_notif, NULL);
   DC_thread_pool_manager_init (&pool_manager, 100, NULL);

   //pthread_create (&thread, NULL, func, NULL);

  /// DC_thread_init (&thread, thread_status, 1000, NULL);
  // DC_thread_run (&thread, thread_func, NULL);
   
   for (i=0; i<120; i++) {
       if (DC_thread_pool_manager_run_task (&pool_manager, task_func, NULL, NULL, NULL, 0) == ERR_OK) {
           //usleep (10000);
           printf ("Allocated thread successfully\n");
       }
   }

   sleep (1000);
   DC_thread_pool_manager_destroy (&pool_manager);
  // DC_thread_destroy (&thread);
   //DC_notifier_notify_all (&thread.PRI (notif_object));
   //DC_notifier_notify_all (&g_notif);
   //DC_notifier_destroy (&g_notif);
 
   return 0;
}
