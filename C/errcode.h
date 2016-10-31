#ifndef _DC_ERRCODE_H
#define _DC_ERRCODE_H

#include "libdc.h"

enum {
    E_OK = 0,
    E_ERROR = -1,
    E_SYSTEM,
    E_FULL,
    E_EMPTY,
    E_TIMEDOUT,
    E_NOMEM,
    E_OUTOFBOUND,
};

#ifdef WINDOWS
#else
#include <errno.h>
#define DC_set_errcode(x) (errno = x)
#define DC_get_errcode()  (errno)
#endif

#endif
