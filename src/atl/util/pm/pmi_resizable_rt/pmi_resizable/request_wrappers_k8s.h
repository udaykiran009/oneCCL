#ifndef REQUEST_WRAPPERS_H
#define REQUEST_WRAPPERS_H

#include <stddef.h>

size_t request_k8s_kvs_init(void);

size_t request_k8s_kvs_get_master(const char* local_host_ip, char* main_host_ip, char* port_str);

size_t request_k8s_kvs_finalize(size_t is_master);

size_t request_k8s_get_replica_size(void);

#endif