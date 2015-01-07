#ifndef _DC_SIGNAL_H
#define _DC_SIGNAL_H

#include "libdc.h"

#include "list.h"

DC_CPP (extern "C" {)

typedef int DC_sig_t;


#ifndef INVALID_SIGNAL
#define INVALID_SIGNAL   ((DC_sig_t)-1)
#endif

#ifdef OS_WINDOWS
#error "The windows platform is not supported now."
#else

extern HDC DC_signal_alloc ();

extern DC_sig_t DC_signal_wait (HDC sig, long milliseconds);

extern int      DC_signal_wait_asyn (HDC hsig,
                                     long milliseconds,
                                     void (*sig_cb)(DC_sig_t, void*),
                                     void *data);
extern int     DC_signal_send (HDC sig, DC_sig_t s);


extern void     DC_signal_free (HDC sig);
#endif

DC_CPP (})
#endif
