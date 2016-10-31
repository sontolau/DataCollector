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
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <assert.h>
typedef uint8_t bool_t;
#endif

typedef uint8_t bool_t;
typedef uint32_t err_t;
typedef unsigned long long OBJ_t;
typedef long long KEY_t;
#ifdef __cplusplus
#define CPP(x)   x
#else
#define CPP(x)
#endif

#if __STDC_VERSION__ >= 199901L || __GNUC__ >= 3
#define INLINE static inline
#define FASTCALL inline
#else
#define INLINE static
#define FASTCALL 
#endif

#ifndef ERR
#define ERR -1
#endif

#ifndef OK
#define OK 0
#endif

#define ISNULL(x) ((x) == NULL)
#define ISERR(x)  ((x) < 0)
#define ISOK(x)   ((x) >= 0)

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif


#ifndef __private__
#define __private__(x)  __##x
#endif

#ifndef __protected__
#define __protected__ _##x
#endif

#ifndef __public__
#define __public__(x)  x
#endif

#ifndef __in__
#define __in__
#endif

#ifndef __out__
#define __out__
#endif

#ifndef __inout__
#define __inout__
#endif

#ifndef MAX
#define MAX(a,b)  (a>b?a:b)
#endif

#ifndef MIN
#define MIN(a,b)  (a>b?b:a)
#endif

#endif
