#include "signal.h"


static HDC g_signal = NULL;

void func(DC_sig_t sig, void *data)
{
    printf ("Receive %d\n", sig);
}

int main ()
{
    g_signal = DC_signal_alloc ();
    if (g_signal == NULL) {
        return -1;
    }
    int i = 0;

    for (i=0; i<10; i++) {
        DC_signal_wait_asyn (g_signal, 0, func, NULL);
    }

    sleep (1);
    DC_signal_send (g_signal, 1111);
    sleep (1);
    DC_signal_send (g_signal, 2222);
    sleep (1);
    DC_signal_free (g_signal);

    sleep (10);
    return 0;
}
