#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kvs_keeper.h"
#include "kube_def.h"

typedef struct kvs_store {
    char name[MAX_KVS_NAME_LENGTH];
    char key[MAX_KVS_KEY_LENGTH];
    char val[MAX_KVS_VAL_LENGTH];
} kvs_store_t;

typedef struct kvs_store_list {
    kvs_store_t kvs;
    struct kvs_store_list* next;
} kvs_store_list_t;

static kvs_store_list_t* head = NULL;
static size_t kvs_list_size = 0;

void put_key(const char kvsname[], const char kvs_key[], const char kvs_val[])
{
    kvs_store_list_t* new_key_ptr = head;

    if (new_key_ptr == NULL)
    {
        head = (kvs_store_list_t*)malloc(sizeof(kvs_store_list_t));
        head->next = NULL;
        new_key_ptr = head;
    }
    else
    {
        while (new_key_ptr->next != NULL)
            new_key_ptr = new_key_ptr->next;

        new_key_ptr->next = (kvs_store_list_t*) malloc(sizeof(kvs_store_list_t));
        new_key_ptr = new_key_ptr->next;
        new_key_ptr->next = NULL;
    }

    strncpy(new_key_ptr->kvs.name, kvsname, MAX_KVS_NAME_LENGTH);
    strncpy(new_key_ptr->kvs.key, kvs_key, MAX_KVS_KEY_LENGTH);
    strncpy(new_key_ptr->kvs.val, kvs_val, MAX_KVS_VAL_LENGTH);

    kvs_list_size++;
}

size_t cut_head(char* kvsname, char* kvs_key, char* kvs_val)
{
    kvs_store_list_t* key_ptr = head;

    if (head != NULL)
    {
        head = head->next;

        strncpy(kvsname, key_ptr->kvs.name, MAX_KVS_NAME_LENGTH);
        strncpy(kvs_key, key_ptr->kvs.key, MAX_KVS_KEY_LENGTH);
        strncpy(kvs_val, key_ptr->kvs.val, MAX_KVS_VAL_LENGTH);

        free(key_ptr);
        kvs_list_size--;
        return 1;
    }
    return 0;
}

size_t get_kvs_list_size()
{
    return kvs_list_size;
}