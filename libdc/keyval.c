#include <json.h>
#include "keyval.h"

static DC_keyval_t *keyval_from_json (json_object*);
static DC_keyval_t *keyval_from_array (json_object *object)
{
    DC_keyval_t *kvs = NULL;
    int length = 0;
    int i = 0;
    json_object *obj = NULL;

    if ((object && (length = json_object_array_length (object)))) {
        return NULL;
    }

    kvs = calloc (length+1, sizeof (DC_keyval_t));
    //DC_keyval_alloc (kvs, length);
    if (kvs == NULL) return NULL;

    for (i=0; i < length; i++) {
        obj = json_object_array_get_idx (object, i);
        switch (json_object_get_type (obj)) {
            case json_type_int: {
                DC_keyval_set (kvs[i], "", KV_TYPE_INT, json_object_get_int (obj));
            } break;
            case json_type_string: {
                DC_keyval_set (kvs[i], "", KV_TYPE_STRING, strdup (json_object_get_string (obj)));
            } break;
            case json_type_object: {
                DC_keyval_set (kvs[i], "", KV_TYPE_KEYVAL, keyval_from_json (obj));
            } break;
            case json_type_array: {
                DC_keyval_set (kvs[i], "", KV_TYPE_ARRAY, keyval_from_array (obj));
            } break;
            case json_type_double: {
                DC_keyval_set (kvs[i], "", KV_TYPE_DOUBLE, json_object_get_double (obj));
            } break;
            case json_type_boolean: {
                DC_keyval_set (kvs[i], "", KV_TYPE_BOOL, json_object_get_boolean (obj));
            } break;
            case json_type_null: {
                DC_keyval_set (kvs[i], "", KV_TYPE_ID, 0);
            } break;
            default: {
                break;
            }
        }
    }

    return kvs;
}

DC_keyval_t *keyval_from_json (json_object *object)
{
    DC_keyval_t *kvs = NULL;
    int length = 0;
    int i      = 0;

    if (!(object && (length = json_object_object_length (object)) > 0)) {
        return NULL;
    }

    kvs = calloc (length+1, sizeof (DC_keyval_t));
    //DC_keyval_alloc (kvs, length);
    if (kvs == NULL) return NULL;

    json_object_object_foreach (object, jkey, jvalue) {
        if (i < length && jkey && jvalue) {
            switch (json_object_get_type (jvalue)) {
                case json_type_int: {
                    DC_keyval_set (kvs[i], jkey, KV_TYPE_INT, json_object_get_int (jvalue));
                }
                    break;
                case json_type_string: {
                    DC_keyval_set (kvs[i], jkey, KV_TYPE_STRING, strdup (json_object_get_string (jvalue)));
                }
                    break;
                case json_type_object: {
                    DC_keyval_set (kvs[i], jkey, KV_TYPE_KEYVAL, keyval_from_json (jvalue));
                }
                    break;
                case json_type_array: {
                    DC_keyval_set (kvs[i], jkey, KV_TYPE_ARRAY, keyval_from_array (jvalue));
                }
                    break;
                case json_type_double: {
                    DC_keyval_set (kvs[i], jkey, KV_TYPE_DOUBLE, json_object_get_double (jvalue));
                } break;
                case json_type_boolean: {
                    DC_keyval_set (kvs[i], jkey, KV_TYPE_BOOL, json_object_get_boolean (jvalue));
                } break;
                case json_type_null: {
                    DC_keyval_set (kvs[i], jkey, KV_TYPE_KEYVAL, NULL);
                } break;
                default:
                    break;
            }
        } else {
            break;
        }
        i++;
    }
    return kvs;
}

static json_object *keyval_to_json (const DC_keyval_t*);
static json_object *keyval_to_array (const DC_keyval_t *kv)
{
    json_object *array_obj = NULL;
    register DC_keyval_t *kvptr;

    DC_keyval_array_foreach (kvptr, kv) {
        if (!array_obj) array_obj = json_object_new_array ();
        switch (kvptr->type) {
            case KV_TYPE_INT: {
                json_object_array_add (array_obj, json_object_new_int (kvptr->int_value));
            } break;
            case KV_TYPE_STRING: {
                json_object_array_add (array_obj, json_object_new_string (kvptr->string_value));
            } break;
            case KV_TYPE_KEYVAL: {
                json_object_array_add (array_obj, keyval_to_json (kvptr->keyval_value));
            } break;
            case KV_TYPE_ARRAY: {
                json_object_array_add (array_obj, keyval_to_array (kvptr->keyval_value));
            } break;
            case KV_TYPE_FLOAT:
            case KV_TYPE_DOUBLE: {
                json_object_array_add (array_obj, json_object_new_double (kvptr->double_value));
            } break;
            case KV_TYPE_BOOL: {
                json_object_array_add (array_obj, json_object_new_boolean (kvptr->bool_value));
            }
            default:
                break;
        }
    }

    return array_obj;
}

json_object *keyval_to_json (const DC_keyval_t *kv)
{
    json_object *root = NULL;
    register DC_keyval_t *kvptr = NULL;

    root = json_object_new_object ();    
    DC_keyval_array_foreach (kvptr, kv) {
        switch (kvptr->type) {
            case KV_TYPE_INT: {
                json_object_object_add (root, kvptr->key, json_object_new_int (kvptr->int_value));
            } break;
            case KV_TYPE_STRING: {
                json_object_object_add (root, kvptr->key, json_object_new_string (kvptr->string_value));
            } break;
            case KV_TYPE_KEYVAL: {
                json_object_object_add (root, kvptr->key, keyval_to_json (kvptr->keyval_value));
            } break;
            case KV_TYPE_ARRAY: {
                json_object_object_add (root, kvptr->key, keyval_to_array (kvptr->keyval_value));
            } break;
            case KV_TYPE_FLOAT:
            case KV_TYPE_DOUBLE: {
                json_object_object_add (root, kvptr->key, json_object_new_double (kvptr->double_value));
            } break;
            case KV_TYPE_BOOL: {
                json_object_object_add (root, kvptr->key, json_object_new_boolean (kvptr->bool_value));
            } break;
            default: {
            } break;
        }
    }

    return root;
}

void DC_keyval_setp (DC_keyval_t *_kv,
                     const char *_key,
                     int _type,
                     unsigned long long _val)
{
    memset (_kv, '\0', sizeof (DC_keyval_t));
    strncpy (_kv->key, _key, MAX_KEY_LEN);
    _kv->type = _type;
    
    switch (_type) {
        case KV_TYPE_BOOL:
            _kv->bool_value = (unsigned char)_val;
            break;
        case KV_TYPE_INT:
             _kv->int_value = (int)_val;
            break;
        case KV_TYPE_FLOAT:
            _kv->float_value = (float)_val;
            break;
        case KV_TYPE_LONG:
            _kv->long_value = (long)_val;
            break;
        case KV_TYPE_DOUBLE:
            _kv->double_value = (double)_val;
            break;
        case KV_TYPE_STRING:
            _kv->string_value = (char*)_val;
            break;
        case KV_TYPE_POINTER:
            _kv->ptr_value  = (void*)_val;
            break;
        case KV_TYPE_KEYVAL:
            _kv->keyval_value = (DC_keyval_t*)_val;
            break;
        case KV_TYPE_ARRAY:
            _kv->keyval_value = (DC_keyval_t*)_val;
            break;
        default:
            break;
    }
}

DC_keyval_t *DC_keyval_array_from_json_string (const char *jsonstr)
{
    json_object *root = NULL;

    root = json_tokener_parse (jsonstr);
    if (!root) {
        return NULL;
    }

    return keyval_from_json (root);
}

const char *DC_keyval_array_to_json_string (const DC_keyval_t *kv)
{
    json_object *json = keyval_to_json (kv);

    return (json?json_object_to_json_string (json):NULL);
}
/*
void DC_keyval_array_loop (DC_keyval_t *kv, int (*cb) (DC_keyval_t*, void*), void *data)
{
    register DC_keyval_t *ptr = NULL;

    DC_keyval_array_foreach (ptr, kv) {
        if (cb && !cb (ptr, data)) {
            return;
        }

        switch (ptr->type) {
            case KV_TYPE_KEYVAL:
            case KV_TYPE_ARRAY:
                DC_keyval_array_loop (ptr->keyval_value, cb, data);
            break;
        }
    }
}
*/
int DC_keyval_array_length (const DC_keyval_t *kvrry)
{
    int len = 0;
    register DC_keyval_t *ptr;

    DC_keyval_array_foreach (ptr, kvrry) {
        len++;
    }
 
    return len;   
}

void DC_keyval_array_copy (DC_keyval_t *dest, const DC_keyval_t *src)
{
    register DC_keyval_t *srcptr, *dstptr;

    if (!(src && dest)) return;

    srcptr = (DC_keyval_t*)src;
    dstptr = dest;

    while (!(srcptr->type == KV_TYPE_NO || dstptr->type == KV_TYPE_NO)) {
        memcpy (dstptr, srcptr, sizeof (DC_keyval_t));
        srcptr++;
        dstptr++;
    }
}

DC_keyval_t *DC_keyval_array_clone (const DC_keyval_t *src)
{
    int len = 0;
    DC_keyval_t *dst = NULL;
    int i = 0;

    if (src == NULL) return NULL;

    len = DC_keyval_array_length (src);
    dst = calloc (len + 1, sizeof (DC_keyval_t));
    //DC_keyval_alloc (dst, len);
    if (dst) {
        for (i=0; i < len; i++) {
            switch (src[i].type) {
                case KV_TYPE_STRING:
                    DC_keyval_set (dst[i], src[i].key, src[i].type, strdup(src[i].string_value));
                    break;
                case KV_TYPE_KEYVAL:
                case KV_TYPE_ARRAY:
                    DC_keyval_set (dst[i], src[i].key, src[i].type, DC_keyval_array_clone (src[i].keyval_value));
                    break;
                default:
                    memcpy (&dst[i], &src[i], sizeof (DC_keyval_t));
                    break;
            }
        }
    }

    return dst;
}

void DC_keyval_free_array (DC_keyval_t *kv)
//void DC_keyval_free_zone (DC_keyval_t *kv)
{
    register DC_keyval_t *ptr;

    if (kv == NULL) return;

    DC_keyval_array_foreach (ptr, kv) {
        switch (ptr->type) {
            case KV_TYPE_STRING:
                free (ptr->string_value);
                break;
            case KV_TYPE_ARRAY:
            case KV_TYPE_KEYVAL:
                DC_keyval_free_array (ptr->keyval_value);
                free (ptr->keyval_value);
                break;
            default:
                break;
        }
    }
}
DC_keyval_t *DC_keyval_find (const DC_keyval_t *kvset, const char *key)
{
    DC_keyval_t *ptr = NULL;
    DC_keyval_array_foreach (ptr, kvset) {
        if (!strcmp (ptr->key, key)) {
            return ptr;
        }
    }

    return NULL;
}

DC_keyval_t *DC_keyval_array_find_path (const DC_keyval_t *kvset, const char *path[])
{
    DC_keyval_t *ptr = NULL;

    ptr = DC_keyval_find (kvset, path[0]);
    if (ptr) {
        if (ptr->type == KV_TYPE_KEYVAL && path[1]) {
            return DC_keyval_array_find_path (ptr->keyval_value, &path[1]);
        } else if (path[1]) {
            return NULL;
        }
    }

    return ptr;
}

