#ifndef _DC_ERROR_H
#define _DC_ERROR_H

#ifndef MAX_ERRBUF_LEN
#define MAX_ERRBUF_LEN 500
#endif

typedef struct _DC_errbuf {
    char errbuf[MAX_ERRBUF_LEN+1];
} DC_errbuf_t;

#define DC_errbuf_set(_ebuf, _estr) strncpy (((DC_errbuf_t*)_ebuf)->errbuf, _estr, strlen (_estr))

#ifdef OS_WINDOWS
static __WIN_errbuf[MAX_ERRBUF_LEN] = {0};
static  char *__WIN_fromat_error (int errcode)
{
    return (char*)__WIN_errbuf;
}
#ifndef ERRNO
#define ERRNO GetLastError()
#endif

#else

#ifndef ERRNO
#define ERRNO errno
#endif

#ifndef ERRSTR
#define ERRSTR strerror(errno)
#endif

#endif

#ifndef ERR_OK
#define ERR_OK   0
#endif

#ifndef ERR_FAILURE
#define ERR_FAILURE -1
#endif

typedef struct _DC_error {
    DC_errbuf_t  text;
    int  code;
} DC_error_t;

#define DC_error_set(_err, _code, _str)        \
    do {\
        if (!_err) break;\
        _err->code = _code;\
        strncpy (_err->text.errbuf, _str, MAX_ERRBUF_LEN);\
    } while (0)

#define DC_error_code(_err)   (_err->code)
#define DC_error_text(_err)   (_err->text.errbuf)

#endif
