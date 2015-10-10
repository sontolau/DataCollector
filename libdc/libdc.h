#ifndef _LIBDC_H
#define _LIBDC_H

#if defined (WIN32) || defined (WINDOWS)
#include <windows.h>
#define OS_WINDOWS
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#ifdef OS_WINDOWS
#else
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <linux/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <assert.h>
#endif

#ifndef SZ_CLASS_NAME
#define SZ_CLASS_NAME 50
#endif

#ifndef SZSHORTSTR
#define SZSHORTSTR 50
#endif


#ifdef __cplusplus
#define DC_CPP(x)   x
#else
#define DC_CPP(x)
#endif

#if __STDC_VERSION__ >= 199901L || __GNUC__ >= 3
#define DC_INLINE static inline
#define FASTCALL inline
#else
#define DC_INLINE static
#define FASTCALL 
#endif

typedef void* HDC;

typedef long long DC_id_t;

#define type_container_of(_ptr, _type, _name) \
    ((_type*)(((unsigned long)_ptr)-(unsigned long)(&(((_type*)0)->_name))))

#define CONTAINER_OF(_ptr, _type, _name) type_container_of(_ptr, _type, _name)
#define P(_type, _x)    ((_type*)&_x)

#ifndef __PRIVATE__
#define __PRIVATE__(x)  __##x
#endif

#ifndef __PUBLIC__
#define __PUBLIC__(x)  x
#endif

#define PRI(x) __PRIVATE__(x)
#define PUB(x) __PUBLIC__(x)


#ifndef __INOUT__
#define __INOUT__
#endif

#ifndef MAX
#define MAX(a,b)  (a>b?a:b)
#endif

#ifndef MIN
#define MIN(a,b)  (a>b?b:a)
#endif


#ifndef Dlog
#define Dlog(_fmt, ...)  \
do {\
    const char *__logenv_##__LINE__=getenv ("DC_LOG");\
    if (!__logenv_##__LINE__ || __logenv_##__LINE__[0] != '1') break;\
    const char *__szenv_##__LINE__ = getenv ("LOG_SIZE");\
    unsigned int __szlog_##__LINE__ = (__szenv_##__LINE__?strtoul (__szenv_##__LINE__, NULL, 10):10*1024*1024);\
    if (ftell (stdout) >= __szlog_##__LINE__) {\
        rewind (stdout);\
        ftruncate (fileno (stdout), 0);\
    }\
\
    fprintf(stdout, _fmt, ## __VA_ARGS__);\
} while (0);

#endif

#include "error.h"

#define CLASS_EXTENDS(__class)    __class PRI(super)

#endif
