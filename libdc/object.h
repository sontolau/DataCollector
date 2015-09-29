#ifndef _DC_OBJECT_H
#define _DC_OBJECT_H

#include "libdc.h"


typedef struct _DC_object {
    int refcount;
    char class_name[SZ_CLASS_NAME+1];
    void *data;
    void (*PRI (release)) (struct _DC_object*, void*);
} DC_object_t;

extern DC_object_t *DC_object_alloc (long size,
                                     const char *cls,
                                     void *data,
                                     DC_object_t* (*__alloc) (void*),
                                     void (*__release)(DC_object_t*, void*));

extern int DC_object_init (DC_object_t *obj);

extern DC_object_t *DC_object_get (DC_object_t *obj);

extern int DC_object_get_refcount (DC_object_t *obj);

extern int DC_object_is_kind_of (DC_object_t *obj, const char *cls);

extern void DC_object_destroy (DC_object_t *obj);

extern void DC_object_release (DC_object_t *);

#define OBJ_EXTENDS(__class)   __class super

#define DC_OBJECT_NEW(__class, __data) \
    (__class*)DC_object_alloc(sizeof(__class), #__class, __data, __class##_alloc, __class##_release)

#define DC_OBJECT_RELEASE(__object) DC_object_release(__object, NULL)

#endif
