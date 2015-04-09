#ifndef _DC_ERROR_H
#define _DC_ERROR_H

//#include "libdc.h"

#ifndef MAX_ERRBUF_LEN
#define MAX_ERRBUF_LEN 500
#endif

#ifdef OS_WINDOWS
static __WIN_errbuf[MAX_ERRBUF_LEN] = {0};
static  char *__WIN_fromat_error (int errcode)
{
    return (char*)__WIN_errbuf;
}
#ifndef ERRNO
#define ERRNO GetLastError()
#endif
#ifndef ERRSTR
#define ERRSTR  __WIN_format_error(ERRNO)
#endif
#else

#ifndef ERRNO
#define ERRNO errno
#endif

#ifndef ERRSTR
#define ERRSTR strerror(errno)
#endif

#endif

enum {
    ERR_OK       = 0,
    ERR_SYSTEM   = -1,
    ERR_TIMEDOUT = -2,
    ERR_NOMEM    = -3,
    ERR_NOTFOUND = -4,
};

extern const char *STRERR(int);

typedef struct _DC_error {
    char *text;
    int  code;
} DC_error_t;

#define DC_error_set(_err, _code, _str)        \
    do {\
        if (!_err) break;\
        _err->code = _code;\
        _err->text = (char*)_str;\
    } while (0)

#endif
