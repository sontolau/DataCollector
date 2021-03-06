#include "keyval.h"

#ifdef USE_JSON_C
#include <json.h>
FASTCALL DC_keyval_t *keyval_from_json(json_object *);

FASTCALL DC_keyval_t *keyval_from_array(json_object *object) {
    DC_keyval_t *kvs = NULL;
    int length = 0;
    int i = 0;
    json_object *obj = NULL;

    if (is_error(object) ||
        (length = json_object_array_length(object)) <= 0) {
        return NULL;
    }

    kvs = calloc(length + 1, sizeof(DC_keyval_t));

    for (i = 0; kvs && i < length; i++) {
        obj = json_object_array_get_idx(object, i);
        switch (json_object_get_type(obj)) {
            case json_type_int: {
                DC_keyval_set (&kvs[i], "", KV_TYPE_INT, json_object_get_int(obj));
            }
                break;
            case json_type_string: {
                DC_keyval_set (&kvs[i], "", KV_TYPE_STRING, json_object_get_string(obj));
            }
                break;
            case json_type_object: {
                DC_keyval_set (&kvs[i], "", KV_TYPE_KEYVAL, keyval_from_json(obj));
            }
                break;
            case json_type_array: {
                DC_keyval_set (&kvs[i], "", KV_TYPE_ARRAY, keyval_from_array(obj));
            }
                break;
            case json_type_double: {
                DC_keyval_set (&kvs[i], "", KV_TYPE_DOUBLE, json_object_get_double(obj));
            }
                break;
            case json_type_boolean: {
                DC_keyval_set (&kvs[i], "", KV_TYPE_BOOL, json_object_get_boolean(obj));
            }
                break;
            case json_type_null: {
                DC_keyval_set (&kvs[i], "", KV_TYPE_KEYVAL, NULL);
            }
                break;
            default: {
                DC_keyval_set (&kvs[i], "", KV_TYPE_ID, 0);
            }
                break;
        }
    }
    DC_keyval_set_zero (&kvs[i]);

    return kvs;
}

DC_keyval_t *keyval_from_json(json_object *object) {
    DC_keyval_t *kvs = NULL;
    int length = 0;
    int i = 0;
    struct json_object_iterator it;
    struct json_object_iterator itEnd;
    struct json_object *obj = object;
    char *jkey;
    struct json_object *jvalue;

    if (is_error(object) ||
        (length = json_object_object_length(object)) <= 0) {
        return NULL;
    }

    kvs = calloc(length + 1, sizeof(DC_keyval_t));

    it = json_object_iter_begin(obj);
    itEnd = json_object_iter_end(obj);

    while (!json_object_iter_equal(&it, &itEnd) && i < length) {
        jkey = (char *) json_object_iter_peek_name(&it);
        jvalue = json_object_iter_peek_value(&it);
        if (jkey) {
            switch (json_object_get_type(jvalue)) {
                case json_type_int: {
                    DC_keyval_set(&kvs[i], jkey, KV_TYPE_INT,
                                  json_object_get_int(jvalue));
                }
                    break;
                case json_type_string: {
                    DC_keyval_set(&kvs[i], jkey, KV_TYPE_STRING,
                                  json_object_get_string(jvalue));
                }
                    break;
                case json_type_object: {
                    DC_keyval_set(&kvs[i], jkey, KV_TYPE_KEYVAL,
                                  keyval_from_json(jvalue));
                }
                    break;
                case json_type_array: {
                    DC_keyval_set(&kvs[i], jkey, KV_TYPE_ARRAY,
                                  keyval_from_array(jvalue));
                }
                    break;
                case json_type_double: {
                    DC_keyval_set(&kvs[i], jkey, KV_TYPE_DOUBLE,
                                  json_object_get_double(jvalue));
                }
                    break;
                case json_type_boolean: {
                    DC_keyval_set(&kvs[i], jkey, KV_TYPE_BOOL,
                                  json_object_get_boolean(jvalue));
                }
                    break;
                case json_type_null: {
                    DC_keyval_set(&kvs[i], jkey, KV_TYPE_KEYVAL, NULL);
                }
                    break;
                default: {
                    DC_keyval_set(&kvs[i], jkey, KV_TYPE_ID, 0);
                }
                    break;
            }
        } else {
            break;
        }
        i++;
        json_object_iter_next(&it);
    }

//    json_object_object_foreach (object, jkey, jvalue) {
//        if (i < length && kvs && jkey && jvalue) {
//            switch (json_object_get_type (jvalue)) {
//                case json_type_int: {
//                    DC_keyval_set (&kvs[i], jkey, KV_TYPE_INT, json_object_get_int (jvalue));
//                }
//                    break;
//                case json_type_string: {
//                    DC_keyval_set (&kvs[i], jkey, KV_TYPE_STRING, json_object_get_string (jvalue));
//                }
//                    break;
//                case json_type_object: {
//                    DC_keyval_set (&kvs[i], jkey, KV_TYPE_KEYVAL, keyval_from_json (jvalue));
//                }
//                    break;
//                case json_type_array: {
//                    DC_keyval_set (&kvs[i], jkey, KV_TYPE_ARRAY, keyval_from_array (jvalue));
//                }
//                    break;
//                case json_type_double: {
//                    DC_keyval_set (&kvs[i], jkey, KV_TYPE_DOUBLE, json_object_get_double (jvalue));
//                } break;
//                case json_type_boolean: {
//                    DC_keyval_set (&kvs[i], jkey, KV_TYPE_BOOL, json_object_get_boolean (jvalue));
//                } break;
//                case json_type_null: {
//                    DC_keyval_set (&kvs[i], jkey, KV_TYPE_KEYVAL, NULL);
//                } break;
//                default: {
//                	DC_keyval_set (&kvs[i], jkey, KV_TYPE_ID, 0);
//                }
//                break;
//            }
//        } else {
//            break;
//        }
//        i++;
//    }
    DC_keyval_set_zero (&kvs[i]);
    return kvs;
}

static json_object *keyval_to_json(const DC_keyval_t *);

static json_object *keyval_to_array(const DC_keyval_t *kv) {
    json_object *array_obj = NULL;
    register DC_keyval_t *kvptr;

    DC_keyval_array_foreach (kvptr, kv) {
        if (!array_obj) array_obj = json_object_new_array();
        switch (kvptr->type) {
            case KV_TYPE_INT: {
                json_object_array_add(array_obj, json_object_new_int(kvptr->int_value));
            }
                break;
            case KV_TYPE_STRING: {
                json_object_array_add(array_obj, json_object_new_string(kvptr->string_value));
            }
                break;
            case KV_TYPE_KEYVAL: {
                json_object_array_add(array_obj, keyval_to_json(kvptr->keyval_value));
            }
                break;
            case KV_TYPE_ARRAY: {
                json_object_array_add(array_obj, keyval_to_array(kvptr->keyval_value));
            }
                break;
            case KV_TYPE_FLOAT:
            case KV_TYPE_DOUBLE: {
                json_object_array_add(array_obj, json_object_new_double(kvptr->double_value));
            }
                break;
            case KV_TYPE_BOOL: {
                json_object_array_add(array_obj, json_object_new_boolean(kvptr->bool_value));
            }
            default:
                break;
        }
    }

    return array_obj;
}

json_object *keyval_to_json(const DC_keyval_t *kv) {
    json_object *root = NULL;
    register DC_keyval_t *kvptr = NULL;

    root = json_object_new_object();
    DC_keyval_array_foreach (kvptr, kv) {
        switch (kvptr->type) {
            case KV_TYPE_INT: {
                json_object_object_add(root, kvptr->key, json_object_new_int(kvptr->int_value));
            }
                break;
            case KV_TYPE_STRING: {
                json_object_object_add(root, kvptr->key, json_object_new_string(kvptr->string_value));
            }
                break;
            case KV_TYPE_KEYVAL: {
                json_object_object_add(root, kvptr->key, keyval_to_json(kvptr->keyval_value));
            }
                break;
            case KV_TYPE_ARRAY: {
                json_object_object_add(root, kvptr->key, keyval_to_array(kvptr->keyval_value));
            }
                break;
            case KV_TYPE_FLOAT:
            case KV_TYPE_DOUBLE: {
                json_object_object_add(root, kvptr->key, json_object_new_double(kvptr->double_value));
            }
                break;
            case KV_TYPE_BOOL: {
                json_object_object_add(root, kvptr->key, json_object_new_boolean(kvptr->bool_value));
            }
                break;
            default: {
            }
                break;
        }
    }

    return root;
}
//
//void DC_keyval_setp(DC_keyval_t *_kv,
//                    const char *_key,
//                    int _type,
//                    unsigned long long _val) {
//    memset(_kv, '\0', sizeof(DC_keyval_t));
//    _kv->type = _type;
//    _kv->key = (char *) _key;
//    switch (_type) {
//        case KV_TYPE_BOOL:
//            _kv->bool_value = (unsigned char) _val;
//            break;
//        case KV_TYPE_INT:
//            _kv->int_value = (int) _val;
//            break;
//        case KV_TYPE_FLOAT:
//            _kv->float_value = (float) _val;
//            break;
//        case KV_TYPE_LONG:
//            _kv->long_value = (long) _val;
//            break;
//        case KV_TYPE_DOUBLE:
//            _kv->double_value = (double) _val;
//            break;
//        case KV_TYPE_STRING:
//            _kv->string_value = (char *) _val;
//            break;
//        case KV_TYPE_POINTER:
//            _kv->ptr_value = (void *) _val;
//            break;
//        case KV_TYPE_KEYVAL:
//            _kv->keyval_value = (DC_keyval_t *) _val;
//            break;
//        case KV_TYPE_ARRAY:
//            _kv->keyval_value = (DC_keyval_t *) _val;
//            break;
//        default:
//            break;
//    }
//}

DC_keyval_t *DC_keyval_from_json(const char *jsonstr) {
    struct json_tokener *tok = NULL;
    json_object *root = NULL;
    enum json_tokener_error jerr;
    DC_keyval_t *kvs = NULL;
    int slen = strlen(jsonstr);

    if (!(tok = json_tokener_new())) return NULL;
    do {
        root = json_tokener_parse_ex(tok, jsonstr, slen);
    } while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);

    if (tok->char_offset < slen) {
    } else if (jerr == json_tokener_success) {
        kvs = keyval_from_json(root);
    } else {
//        Elog("%s", json_tokener_error_desc(jerr));
    }

    json_tokener_free(tok);
    return kvs;
}

char *DC_keyval_to_json(const DC_keyval_t *kv) {
    json_object *json = keyval_to_json(kv);

    return (json ? json_object_to_json_string(json) : NULL);
}
#endif

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
uint32_t DC_keyval_get_length(const DC_keyval_t *kvrry) {
    int i = 0;

    for (i = 0; !KV_IS_NULL(kvrry[i]); ++i);

    return ++i;
}
//
//void DC_keyval_array_copy (DC_keyval_t *dest, const DC_keyval_t *src)
//{
//    register DC_keyval_t *srcptr, *dstptr;
//
//    if (!(src && dest)) return;
//
//    srcptr = (DC_keyval_t*)src;
//    dstptr = dest;
//
//    while (!(srcptr->type == KV_TYPE_NO || dstptr->type == KV_TYPE_NO)) {
//        memcpy (dstptr, srcptr, sizeof (DC_keyval_t));
//        srcptr++;
//        dstptr++;
//    }
//}
//
//DC_keyval_t *DC_keyval_array_clone (const DC_keyval_t *src)
//{
//    int len = 0;
//    DC_keyval_t *dst = NULL;
//    int i = 0;
//
//    if (src == NULL) return NULL;
//
//    len = DC_keyval_array_length (src);
//    dst = calloc (len + 1, sizeof (DC_keyval_t));
//    //DC_keyval_alloc (dst, len);
//    if (dst) {
//        for (i=0; i < len; i++) {
//            switch (src[i].type) {
//                case KV_TYPE_STRING:
//                    DC_keyval_set (dst[i], src[i].key, src[i].type, src[i].string_value);
//                    break;
//                case KV_TYPE_KEYVAL:
//                case KV_TYPE_ARRAY:
//                    DC_keyval_set (dst[i], src[i].key, src[i].type, DC_keyval_array_clone (src[i].keyval_value));
//                    break;
//                default:
//                    memcpy (&dst[i], &src[i], sizeof (DC_keyval_t));
//                    break;
//            }
//        }
//    }
//
//    return dst;
//}

void DC_keyval_iterator(DC_keyval_t *kvset,
                        void (*cb)(DC_keyval_t *, void *),
                        void *data) {
    int i = 0;

    for (i = 0; !KV_IS_NULL(kvset[i]); i++) {

    }
}

DC_keyval_t *DC_keyval_find(DC_keyval_t *kvset, const char *key) {
    DC_keyval_t *ptr = NULL;
    int i;

    for (i = 0; KV_IS_NULL(kvset[i]); i++) {
        if (!strcmp(ptr->key, key)) {
            return &kvset[i];
        }
    }

    return NULL;
}

DC_keyval_t *DC_keyval_find_path(DC_keyval_t *kvset, const char *const path[]) {
    DC_keyval_t *ptr = NULL;

    if ((ptr = DC_keyval_find(kvset, path[0]))) {
        if (ptr->type == KV_TYPE_KEYVAL && path[1]) {
            return DC_keyval_find_path(ptr->keyval_value, &path[1]);
        } else {
            return NULL;
        }
    }

    return ptr;
}

//static inline const char *int2str(int x) {
//    static char buf[100] = {0};
//
//    snprintf(buf, sizeof(buf), "%d", x);
//
//    return buf;
//}
//
//
//void DC_keyval_array_get_string3(const DC_keyval_t *kvs,
//                                 char **keys,
//                                 char **values,
//                                 char **kvstr,
//                                 const char *sep) {
//    register DC_keyval_t *kvptr;
//    int szkeys = 0;
//    int szvals = 0;
//    int szkvs = 0;
//    int szsep = strlen(sep);
//
//    DC_keyval_array_foreach (kvptr, ((DC_keyval_t *) kvs)) {
//        szkeys += strlen(kvptr->key);
//        szkeys += szsep;
//
//        switch (kvptr->type) {
//            case KV_TYPE_INT:
//                szvals += strlen(int2str(kvptr->int_value));
//                break;
//            case KV_TYPE_STRING:
//                szvals += strlen(kvptr->string_value) + 2;
//                break;
//            default:
//                continue;
//                break;
//        }
//        szvals += szsep;
//
//        szkvs += (szkeys + szvals + 1 + szsep);
//    }
//
//    if (keys)
//        *keys = calloc(1, szkeys + 10);
//    if (values)
//        *values = calloc(1, szvals + 10);
//    if (kvstr)
//        *kvstr = calloc(1, szkvs + 10);
//
//    DC_keyval_array_foreach (kvptr, kvs) {
//        if (keys && *keys) {
//            sprintf(*keys, "%s%s%s", *keys, kvptr->key, sep);
//        }
//
//        //if (*values) {
//        switch (kvptr->type) {
//            case KV_TYPE_INT: {
//                if (values && *values) {
//                    sprintf(*values, "%s%d%s", *values, kvptr->int_value, sep);
//                }
//                if (kvstr && *kvstr) {
//                    sprintf(*kvstr, "%s%s=%d%s", *kvstr, kvptr->key, kvptr->int_value, sep);
//                }
//            }
//                break;
//            case KV_TYPE_STRING: {
//                if (values && *values) {
//                    sprintf(*values, "%s\'%s\'%s", *values, kvptr->string_value, sep);
//                }
//                if (kvstr && *kvstr) {
//                    sprintf(*kvstr, "%s%s=\'%s\'%s", *kvstr, kvptr->key, kvptr->string_value, sep);
//                }
//            }
//                break;
//            default:
//                break;
//        }
//        // }
//    }
//
//    if (keys && *keys) {
//        (*keys)[strlen(*keys) - szsep] = '\0';
//    }
//
//    if (values && *values) {
//        (*values)[strlen(*values) - szsep] = '\0';
//    }
//
//    if (kvstr && *kvstr) {
//        (*kvstr)[strlen(*kvstr) - szsep] = '\0';
//    }
//}

//void DC_keyval_array_release(DC_keyval_t *map, void (cb)(DC_keyval_t *)) {
//    register DC_keyval_t *maptr = NULL;
//
//    DC_keyval_array_foreach (maptr, map) {
//        if (cb) {
//            cb(maptr);
//        }
//    }
//}
