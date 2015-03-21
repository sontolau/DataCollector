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

#ifdef OS_WINDOWS
#else
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
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
#else
#define DC_INLINE static
#endif

typedef void* HDC;

#define type_container_of(_ptr, _type, _name) \
    ((_type*)(((unsigned long)_ptr)-(unsigned long)(&(((_type*)0)->_name))))

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

#ifndef Dlog
#define Dlog(_fmt, ...)  fprintf(stderr, _fmt, ## __VA_ARGS__)
#endif

#include "error.h"
#include "link.h"
#include "dict.h"
#include "hash.h"
#include "list.h"
#include "queue.h"
#include "keyval.h"
#include "buffer.h"
#include "mutex.h"
#include "thread.h"
#include "notifier.h"
#include "netio.h"

#endif
