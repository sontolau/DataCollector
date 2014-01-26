#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dict.h"


static unsigned int __dict_index_func (void *key) {
    char *strkey = (char*)key;
    int i;
    unsigned short lo = 1, hi = 0;

    for (i=0; i<strlen (strkey); i++) {
        lo = lo * strkey[i];
        hi = hi + strkey[i];
    }

    return (unsigned int)(hi<<16 | lo);
}

static int __dict_compare_func (void *obj, void *key) {
    void **keyobj = (void**)obj;
    char *strkey = (char*)keyobj[0];

    if (!strcmp (strkey, (char*)key)) {
        return 1;
    }

    return 0;
}

static void __dict_destroy_func (void *obj) {
    free (obj);
}


int DC_dict_init (DC_dict_t *dic, ...) {
    va_list ap;
    char *key;
    void *arg, *obj;

    if (DC_hash_init (dic, 100, __dict_index_func,
                      __dict_compare_func,
                      __dict_destroy_func) < 0) {
        return -1;
    }

    va_start (ap, dic);
    key = NULL;
    while (1) {
        if ((arg = va_arg (ap, void*))) {
            if (key == NULL) {
                key = arg;
            } else {
                obj = arg;
                DC_dict_add_object_with_key (dic, key, obj);
                key = NULL;
            }
        } else {
            break;
        }
    }

    return 0;
}

int DC_dict_add_object_with_key (DC_dict_t *dic, const char *key, const void *obj) {
    void **keyobj;

    keyobj = (void**)calloc (2, sizeof (void*));
    if (keyobj == NULL) {
        return -1;
    }

    keyobj[0] = (char*)key;
    keyobj[1] = (void*)obj;

    return DC_hash_add_object (dic, (char*)key, (void*)keyobj);
}

void *DC_dict_get_object_with_key (DC_dict_t *dic, const char *key)
{
    void **keyobj;

    keyobj = DC_hash_get_object (dic, (char*)key);
    if (keyobj == NULL) {
        return NULL;
    }

    return keyobj[1];
}

void DC_dict_remove_object_with_key (DC_dict_t *dic, const char *key) {
    DC_hash_remove_object (dic, (char*)key);
}

void DC_dict_destroy (DC_dict_t *dic) {
    DC_hash_destroy (dic);
}
