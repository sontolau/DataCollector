#ifndef _DC_KEYVAL_H
#define _DC_KEYVAL_H

#ifndef MAX_KEY_LEN
#define MAX_KEY_LEN 255
#endif

enum {
    KV_TYPE_CHAR = 1,
    KV_TYPE_SHORT = 2,
    KV_TYPE_INT,
    KV_TYPE_FLOAT,
    KV_TYPE_LONG,
    KV_TYPE_ID,
    KV_TYPE_DOUBLE,
    KV_TYPE_STRING,
    KV_TYPE_POINTER,
};

typedef struct _DC_keyval {
    char key[MAX_KEY_LEN+1];
    int  type;
    union {
        char          char_value;
        short         short_value;
        int           int_value;
        float         float_value;
        long          long_value;
        unsigned long long id_value;
        double        double_value;
        char          *string_value;
        void          *ptr_value;
    };
} DC_keyval_t;


#define DC_keyval_setp(_kv, _key, _type, _val) \
    do {\
        memset (_kv, '\0', sizeof (DC_keyval_t));\
        strncpy (_kv->key, _key, MAX_KEY_LEN);\
        _kv->type = _type;\
        switch (_type) {\
            case KV_TYPE_CHAR:\
                _kv->char_value = (char)((long long)_val);\
                break;\
            case KV_TYPE_SHORT:\
                _kv->short_value = (short)((long long)_val);\
                break;\
            case KV_TYPE_INT:\
                _kv->int_value = (int)((long long)_val);\
                break;\
            case KV_TYPE_FLOAT:\
                _kv->float_value = (float)((long long)_val);\
                break;\
            case KV_TYPE_LONG:\
                _kv->long_value = (long)((long long)_val);\
                break;\
            case KV_TYPE_DOUBLE:\
                _kv->double_value = (double)((long long)_val);\
                break;\
            case KV_TYPE_STRING:\
                _kv->string_value = (char*)((long long)_val);\
                break;\
            case KV_TYPE_POINTER:\
                _kv->ptr_value  = (void*)((long long)_val);\
                break;\
            default:\
                break;\
        }\
    } while (0)

#define DC_keyval_set(_kv, _key, _type, _val) \
    DC_keyval_setp(((DC_keyval_t*)&_kv), _key, _type, _val)
#endif
