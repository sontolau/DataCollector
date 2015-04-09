#include "libdc.h"


const char *STRERR (int err) {
    const char *__errstr__[] = {
        [-ERR_OK]       = "Success",
        [-ERR_SYSTEM]   = ERRSTR,
        [-ERR_TIMEDOUT] = "Operation timedout",
    };

    return __errstr__[err];
}

