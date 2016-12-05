#if 0
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

#endif

#ifdef LIST_DEBUG

struct Node {
    DC_list_elem_t list;
    char name[255];
};

void print_list (DC_list_t *list)
{
    DC_list_elem_t *elem = NULL;
    void *saveptr = NULL;

    printf ("There are %d elements: ( ", list->count);
    while ((elem = DC_list_next_object (list, &saveptr))) {
        printf ("%s, ", CONTAINER_OF(elem,struct Node, list)->name);
    }
    printf (" )\n");
}

int main () {
    struct Node nodes[10];
    DC_list_elem_t *elem = NULL;
    struct Node *nodeptr;
    int i = 0;

    void *saveptr = NULL;

    DC_list_t list;
    if (DC_list_init (&list, NULL, NULL, NULL) < 0) {
        printf ("DC_list_init failed\n");
        exit (1);
    }

    for (i=0; i<10; i++) {
        sprintf (nodes[i].name, "%d", i);
        DC_list_add_object (&list, &nodes[i].list);
    }

    print_list (&list);
    printf ("Insert 56 at index 8\n");
    struct Node tmpnode;

    sprintf (tmpnode.name, "56");
    DC_list_insert_object_at_index (&list, &tmpnode.list, 8);
    print_list (&list);

    elem = DC_list_get_object_at_index (&list, 10);
    printf ("The element that at the index of 11 is %s\n", CONTAINER_OF(elem, struct Node, list)->name);

    printf ("Remove the object at index 5\n");
    DC_list_remove_object_at_index (&list, 5);
    print_list (&list);
    printf ("Remove all objects\n");
    DC_list_remove_all_objects (&list);
    print_list (&list);

    DC_list_destroy (&list);
    return 0;
}


#endif

#ifdef THREAD_DEBUG
#include "thread.h"

void __thread_func (void *data)
{
    int i = 0;
    while (1) {
    i++;
    printf (" %d The status is : %d\n", (int)data, time (NULL)); sleep (1);
    if (i>10) break;}
}

int main ()
{
/*
    DC_thread_t thread;

    DC_thread_init (&thread);
    DC_thread_run (&thread, __thread_func, &thread);
    sleep (10);
    DC_thread_destroy (&thread);
    sleep (1);
*/
    int ret;
    DC_task_manager_t manager;

    DC_task_manager_init (&manager, 3);
    int i;
    for (i=0; i<5; i++) {
        ret = DC_task_manager_run_task (&manager, __thread_func, (void*)i, 1);
        if (ret == ERR_BUSY) {
            printf ("Can not run the task at index: %d\n", i);
        }
    }

    sleep (10);
    DC_task_manager_destroy (&manager);
    return 0;
}

#endif
