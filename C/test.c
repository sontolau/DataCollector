#include "queue.h"
#include <signal.h>

DC_queue_t queue;
unsigned int x = 0;
unsigned int y = 0;
volatile int ext = 0;

void sig(int s)
{
    if (s == SIGINT) {
        if (ext == 1) exit (0);
        ext = 1;
        return;
    }

    printf ("Writing count: %u Reading count: %u\n", x, y);
    alarm (1);
}

void *print_func(void *data) {
    DC_queue_object_t e;

    while ((e = DC_queue_fetch (&queue, TRUE, 0)) != (DC_queue_object_t)NULL) {
        y++;
        //fprintf (stderr, "Reading: %u\n", e);
    }
}

void main() {
   DC_thread_t t;
   time_t tm;

   DC_thread_create (t, print_func, NULL);
   DC_queue_init (&queue, 100, 0);
   signal (SIGALRM, sig);
   //signal (SIGINT, sig);
   alarm (1);
   
   usleep (1);
   while (!ext) {
       tm = (time(NULL)/1000);
       //fprintf (stderr, "Writing : %u\n", t);
       if (ISERR (DC_queue_add (&queue, (DC_queue_object_t)tm, TRUE, 0))) {
           fprintf (stderr, "Queue is error\n");
           break;
       }
       x++;
   }

   sleep (100);
}
