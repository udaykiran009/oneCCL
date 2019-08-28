#ifndef REQUEST_WRAPPERS_H
#define REQUEST_WRAPPERS_H

#include <stddef.h>

size_t request_k8s_kvs_init(void);

size_t request_k8s_kvs_finalize(void);

size_t request_k8s_get_keys_values_by_name(const char* kvs_name, char*** kvs_key, char*** kvs_values);

size_t request_k8s_get_count_names(const char* kvs_name);

size_t request_k8s_get_val_by_name_key(const char* kvs_name, const char* kvs_key, char* kvs_val);

size_t request_k8s_remove_name_key(const char* kvs_name, const char* kvs_key);

size_t request_k8s_set_val(const char* kvs_name, const char* kvs_key, const char* kvs_val);

size_t request_k8s_get_replica_size(void);

#endif