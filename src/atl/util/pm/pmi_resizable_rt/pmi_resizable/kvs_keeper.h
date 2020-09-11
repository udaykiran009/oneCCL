#ifndef KVS_KEEPER_H_INCLUDED
#define KVS_KEEPER_H_INCLUDED

typedef enum storage_type {
    ST_CLIENT = 0,
    ST_SERVER = 1,
} storage_type_t;

size_t get_count(const char kvs_name[], storage_type_t st_type);

size_t get_val(const char kvs_name[], const char kvs_key[], char* kvs_val, storage_type_t st_type);

size_t get_keys_values(const char* kvs_name,
                       char*** kvs_keys,
                       char*** kvs_values,
                       storage_type_t st_type);

size_t remove_val(const char kvs_name[], const char kvs_key[], storage_type_t st_type);

void put_key(const char kvs_name[],
             const char kvs_key[],
             const char kvs_val[],
             storage_type_t st_type);

void kvs_keeper_clear(storage_type_t st_type);

size_t cut_head(char* kvs_name, char* kvs_key, char* kvs_val, storage_type_t st_type);

size_t get_kvs_list_size(storage_type_t st_type);

#endif
