#ifndef _DC_KEYVAL_H
#define _DC_KEYVAL_H

#include "libdc.h"

enum {
    KV_TYPE_NO = 0,
    KV_TYPE_BOOL = 1,
    KV_TYPE_INT = 2,
    KV_TYPE_LONG,
    KV_TYPE_ID,
    KV_TYPE_FLOAT,
    KV_TYPE_DOUBLE,
    KV_TYPE_STRING,
    KV_TYPE_KEYVAL,
    KV_TYPE_ARRAY,
    KV_TYPE_POINTER,
};

typedef struct _DC_keyval {
    char *key;
    int type;
    union {
        unsigned char bool_value;
        int int_value;
        float float_value;
        long long_value;
        ID_t id_value;
        double double_value;
        char *string_value;
        struct _DC_keyval *keyval_value;
        void *ptr_value;
    };
} DC_keyval_t;

#define KV(key, type, val) (DC_keyval_t){.key=key, .type=type, .id_value=(ID_t)val}
#define KV_NULL KV(NULL, 0, 0)
#define KV_IS_NULL(kv) (!(kv).key)

#ifdef USE_JSON_C
extern DC_keyval_t *DC_keyval_from_json(const char *jsonstr);

extern void DC_keyval_free_json(__in__ DC_keyval_t *keyval);

extern const char *DC_keyval_to_json(__in__ const DC_keyval_t *);
#endif

extern void DC_keyval_iterator(__in__ DC_keyval_t *kvset,
                               __in__ void (*cb)(DC_keyval_t *kv, void *),
                               __in__ void *data);

extern DC_keyval_t *DC_keyval_find(__in__ DC_keyval_t *, __in__ const char *);

extern DC_keyval_t *DC_keyval_find_path(__in__ DC_keyval_t *, __in__ const char *const path[]);

#endif
