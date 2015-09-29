#ifndef _DC_ERROR_H
#define _DC_ERROR_H

#ifndef MAX_ERRBUF_LEN
#define MAX_ERRBUF_LEN 500
#endif

enum {
    ERR_OK = 0,
    ERR_FAILURE = -1,
    ERR_NOMEM = -2,
    ERR_FULL = -3,
    ERR_BUSY = -4,
    ERR_TIMEDOUT,
};

#ifdef OS_WINDOWS
#ifndef ERRNO
#define ERRNO GetLastError()
#endif

#else

#ifndef ERRNO
#define ERRNO errno
#endif

#endif

#endif
