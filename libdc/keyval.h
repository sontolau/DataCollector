#ifndef _DC_KEYVAL_H
#define _DC_KEYVAL_H

#include "libdc.h"

#ifndef MAX_KEY_LEN
#define MAX_KEY_LEN 255
#endif

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
    char key[MAX_KEY_LEN+1];
    int  type;
    union {
        unsigned char bool_value;
        int           int_value;
        float         float_value;
        long          long_value;
        unsigned long long id_value;
        double        double_value;
        char          *string_value;
        struct _DC_keyval *keyval_value;
        void          *ptr_value;
    };
} DC_keyval_t;


extern void DC_keyval_setp (DC_keyval_t *kv,
                            const char *key,
                            int type,
                            unsigned long long val);

#define DC_keyval_set(_kv, _key, _type, _val) \
    DC_keyval_setp(((DC_keyval_t*)&_kv), _key, _type, (unsigned long long)_val)

//#define DC_keyval_eof(_kvptr)  (_kvptr->type == KV_TYPE_NO)
#define DC_keyval_array_foreach(_kvptr, _kvarry) \
            for(_kvptr=(DC_keyval_t*)(&_kvarry[0]); _kvarry && _kvptr->type != KV_TYPE_NO; _kvptr++)

extern  DC_keyval_t *DC_keyval_array_from_json_string (const char *jsonstr);

extern void DC_keyval_free_array (DC_keyval_t *keyval);

#define DC_keyval_array_set(_kv, _i, _key, _type, _val) DC_keyval_set(_kv[_i], _key, _type, _val)

extern const char *DC_keyval_array_to_json_string (const DC_keyval_t*);

extern int DC_keyval_array_length (const DC_keyval_t*);

//extern void DC_keyval_copy (DC_keyval_t *dest, const DC_keyval_t *src);

extern DC_keyval_t *DC_keyval_array_find (const DC_keyval_t*, const char*);

extern DC_keyval_t *DC_keyval_array_find_path (const DC_keyval_t*, const char *path[]);

extern DC_keyval_t *DC_keyval_array_clone (const DC_keyval_t *src);

/*
extern void DC_keyval_free_zone (DC_keyval_t *src);

#define DC_keyval_free(kvs) \
do{\
    DC_keyval_free_zone (kvs);\
    free (kvs);\
} while (0)
*/
#endif