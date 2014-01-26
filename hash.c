#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "list.h"
#include "hash.h"


struct hash_carrier {
    void *key;
    void *obj;
};

static DC_list_t *__list_entry (DC_list_t **map, 
                                unsigned int size,
                                void *key, 
                                hash_id_func_t id_func) {
    unsigned int id;

    if (id_func == NULL) {
        return NULL;
    }

    id = (unsigned int)(id_func (key)%size);
    if (map[id] == NULL) {
        return NULL;
    }

    return &map[id][id];
}

static struct hash_carrier *__new_hash_carrier (void *key, void *obj) {
    struct hash_carrier *hc = NULL;

    hc = (struct hash_carrier*)malloc (sizeof (struct hash_carrier));
    if (hc) {
        hc->key = key;
        hc->obj = obj;
    }
    return hc;
}

static struct hash_carrier *__find_hash_carrier (DC_hash_t *hash, void *key) {
    DC_list_t *list;
    void *saveptr = NULL;
    struct hash_carrier *hc;

    if (hash->__hash_compare == NULL) {
        return NULL;
    }

    list = __list_entry ((DC_list_t**)hash->__hash_map,
                         hash->size,
                         key,
                         hash->__hash_id);
    if (list) {
        while ((hc = DC_list_next_object (list, &saveptr))) {
            if (hash->__hash_compare (hc->obj, hc->key)) {
                return hc;
            }
        }
    }

    return NULL;
}

int DC_hash_init (DC_hash_t *hash, 
                  int size,
                  hash_id_func_t id_func,
                  hash_compare_func_t comp_func,
                  hash_destroy_func_t dest_func) {

    DC_list_t **list_map;
    int i, j;

    memset (hash, '\0', sizeof (DC_hash_t));
    list_map = (DC_list_t**)calloc (size, sizeof (DC_list_t*));
    if (list_map == NULL)  {
        return -1;
    }

    for (i=0; i<size; i++) {
        list_map[i] = (DC_list_t*)calloc (size, sizeof (DC_list_t));
        for (j=0; j<size; j++) {
            DC_list_init (&list_map[i][j], NULL);
        }
    }

    hash->size = size;
    hash->num_objects = 0;
    hash->__hash_map = (void**)list_map;
    hash->__hash_id = id_func;
    hash->__hash_compare = comp_func;
    hash->__hash_destroy = dest_func;

    return 0;
}

int DC_hash_add_object (DC_hash_t *hash, void *key, void *obj) {
    DC_list_t *list;
    struct hash_carrier *hc;

    list = __list_entry ((DC_list_t**)hash->__hash_map, 
                         hash->size, 
                         key,
                         hash->__hash_id);

    if (list == NULL) {
        return -1;
    }

    if (!(hc = __new_hash_carrier (key, obj))) {
        return -1;
    }
    
    DC_list_add (list, (void*)hc);
    hash->num_objects++;

    return 0;
}


void *DC_hash_get_object (DC_hash_t *hash, void *key) {
    struct hash_carrier *hc;

    hc = __find_hash_carrier (hash, key);
    return hc?hc->obj:NULL;
}

void DC_hash_remove_object (DC_hash_t *hash, void *key) {
    struct hash_carrier *hc;
    DC_list_t *list;

    list = __list_entry ((DC_list_t**)hash->__hash_map,
                          hash->size,
                          key,
                          hash->__hash_id);

    if (list && (hc = __find_hash_carrier (hash, key))) {
        hash->num_objects--;
        DC_list_remove_object (list, (void*)hc);
        if (hash->__hash_destroy) {
            hash->__hash_destroy (hc->obj);
        }
        free (hc);
    }
}

void DC_hash_destroy (DC_hash_t *hash) {
    int i, j;
    DC_list_t **listmap;

    listmap = (DC_list_t**)hash->__hash_map;
    for (i=0; i<hash->size; i++) {
        for (j=0; j<hash->size; j++) {
            DC_list_destroy (&listmap[i][j]);
        }
    }
}

