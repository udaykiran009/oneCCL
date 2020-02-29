#ifndef KVS
#define KVS

#include <stddef.h>

size_t kvs_set_value(const char* kvs_name, const char* kvs_key, const char* kvs_val);

size_t kvs_remove_name_key(const char* kvs_name, const char* kvs_key);

size_t kvs_get_value_by_name_key(const char* kvs_name, const char* kvs_key, char* kvs_val);

size_t kvs_init(const char* main_addr);

size_t main_server_address_reserve(char* main_addr);

size_t kvs_get_count_names(const char* kvs_name);

size_t kvs_finalize(void);

size_t kvs_get_keys_values_by_name(const char *kvs_name, char ***kvs_keys, char ***kvs_values);

size_t kvs_get_replica_size(void);

#endif