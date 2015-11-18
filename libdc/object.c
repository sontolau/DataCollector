#include "object.h"

DC_object_t *DC_object_alloc (long size,
		const char *cls,
		void *data,
		DC_object_t* (*__alloc) (const char*, void*),
		void (*__release)(DC_object_t*, void*)) {
	DC_object_t *obj = NULL;

    assert (__release != NULL);
    if (!__alloc) {
	    obj = (DC_object_t*) calloc(1, size);
    } else {
        obj = __alloc (cls, data);
    }

    if (obj) {
        obj->data     = data;
        obj->refcount = 1;
        strncpy(obj->class_name, cls, sizeof(obj->class_name) - 1);
        obj->PRI (release) = __release;
        obj->PRI (alloc)   = __alloc;
/*
        if (__init && __init (obj, data) < 0) {
            obj->PRI (release) (obj, data);
            return NULL;
        }
*/
    }
	return obj;
}

int DC_object_init (DC_object_t *obj)
{
    return ERR_OK;
}

int DC_object_is_kind_of(DC_object_t *obj, const char *cls) {
	return strncmp(obj->class_name, cls, SZ_CLASS_NAME) ? 0 : 1;
}


void DC_object_release(DC_object_t *obj) {
	if (obj) {
		obj->refcount--;
		if (obj->refcount <= 0) {
            obj->__release (obj, obj->data);
            if (!obj->PRI (alloc)) {
                free (obj);
            }
		}
	}
}

DC_object_t *DC_object_get(DC_object_t *obj) {
	if (obj) {
		obj->refcount++;
	}

	return obj;
}

int DC_object_get_refcount(DC_object_t *obj) {
	return obj ? obj->refcount : 0;
}
