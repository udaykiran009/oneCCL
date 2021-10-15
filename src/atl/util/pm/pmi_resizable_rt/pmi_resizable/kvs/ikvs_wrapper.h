#pragma once

#include <unistd.h>
#include "util/pm/pmi_resizable_rt/pmi_resizable/def.h"

class ikvs_wrapper {
public:
    virtual ~ikvs_wrapper() noexcept(false){};

    virtual kvs_status_t kvs_set_value(const char* kvs_name,
                                       const char* kvs_key,
                                       const char* kvs_val) = 0;

    virtual kvs_status_t kvs_remove_name_key(const char* kvs_name, const char* kvs_key) = 0;

    virtual kvs_status_t kvs_get_value_by_name_key(const char* kvs_name,
                                                   const char* kvs_key,
                                                   char* kvs_val) = 0;

    virtual kvs_status_t kvs_init(const char* main_addr) = 0;

    virtual kvs_status_t kvs_main_server_address_reserve(char* main_addr) = 0;

    virtual kvs_status_t kvs_get_count_names(const char* kvs_name, int& count_names) = 0;

    virtual kvs_status_t kvs_finalize() = 0;

    virtual kvs_status_t kvs_get_keys_values_by_name(const char* kvs_name,
                                                     char*** kvs_keys,
                                                     char*** kvs_values,
                                                     size_t& count) = 0;

    virtual kvs_status_t kvs_get_replica_size(size_t& replica_size) = 0;
};
