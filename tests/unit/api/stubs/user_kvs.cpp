
#include <string.h>
#include "util/pm/pmi_resizable_rt/pmi_resizable/kvs/users_kvs.h"
#include "util/pm/pmi_resizable_rt/pmi_resizable/def.h"

users_kvs::users_kvs(std::shared_ptr<ccl::kvs_interface> kvs) {}

size_t users_kvs::kvs_set_value(const char* kvs_name, const char* kvs_key, const char* kvs_val) {
    return 0;
}

size_t users_kvs::kvs_remove_name_key(const char* kvs_name, const char* kvs_key) {
    return 0;
}

size_t users_kvs::kvs_get_value_by_name_key(const char* kvs_name,
                                            const char* kvs_key,
                                            char* kvs_val) {
    return 0;
}

size_t users_kvs::kvs_get_count_names(const char* kvs_name) {
    return 0;
}

size_t users_kvs::kvs_get_keys_values_by_name(const char* kvs_name,
                                              char*** kvs_keys,
                                              char*** kvs_values) {
    /*TODO: Unsupported*/
    (void)kvs_name;
    (void)kvs_keys;
    (void)kvs_values;
    return 0;
}

size_t users_kvs::kvs_get_replica_size(void) {
    /*TODO: Unsupported*/
    return 0;
}

size_t users_kvs::kvs_main_server_address_reserve(char* main_address) {
    /*TODO: Unsupported*/
    (void)main_address;
    return 0;
}

size_t users_kvs::kvs_init(const char* main_addr) {
    /*TODO: Unsupported*/
    (void)main_addr;
    return 0;
}

size_t users_kvs::kvs_finalize(void) {
    /*TODO: Unsupported*/
    return 0;
}
