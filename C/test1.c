#include "keyval.h"

int main ()
{
    const char *json = "{\"name\":\"sonto\", \"age\":23, \"sex\": 1}";
    DC_keyval_t kv_head;

    DC_keyval_init (&kv_head, "name", KV_TYPE_STRING, "sonto.lau");
    DC_keyval_put (&kv_head, "age", KV_TYPE_INT, 28);
    DC_keyval_put (&kv_head, "sex", KV_TYPE_INT, 1);

    printf ("%s, %d, %d\n", DC_keyval_get (&kv_head, "name")->string_value, DC_keyval_get (&kv_head, "age")->int_value, DC_keyval_get (&kv_head, "sex")->int_value);

    DC_keyval_t *kvptr;

    kvptr = DC_keyval_from_json_string (json);
    void *saveptr;
    DC_keyval_t *v;

    saveptr = kvptr;
    DC_keyval_remove (kvptr, "age");
    while ((v = DC_keyval_next (saveptr, &saveptr))) {
        printf ("%s\n", v->key);
    }

    DC_keyval_json_free (kvptr);
    return 0;
}
