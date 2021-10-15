#ifndef REQUEST_WRAPPERS_H
#define REQUEST_WRAPPERS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>

kvs_status_t request_k8s_kvs_init(void);

kvs_status_t request_k8s_kvs_get_master(const char* local_host_ip,
                                        char* main_host_ip,
                                        char* port_str);

kvs_status_t request_k8s_kvs_finalize(size_t is_master);

kvs_status_t request_k8s_get_replica_size(size_t& res);

#ifdef __cplusplus
}
#endif
#endif