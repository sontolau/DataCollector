#include "error.h"


const char *ERRSTR (int err) {
    const char *__errstr__[] = {
        [-ERR_OK]       = "Success",
        [-ERR_SYSTEM]   = ERRSYS,
        [-ERR_TIMEDOUT] = "Operation timedout",
    };

    return __errstr__[err];
}

