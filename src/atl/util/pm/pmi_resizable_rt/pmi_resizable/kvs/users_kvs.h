#pragma once
#include <unistd.h>
#include <memory>
#include "oneapi/ccl.hpp"
#include "util/pm/pmi_resizable_rt/pmi_resizable/kvs/ikvs_wrapper.h"

class users_kvs final : public ikvs_wrapper {
public:
    users_kvs() = delete;
    users_kvs(std::shared_ptr<ccl::kvs_interface> kvs) : kvs(kvs){};

    ~users_kvs() = default;

    size_t kvs_set_value(const char* kvs_name, const char* kvs_key, const char* kvs_val) override;

    size_t kvs_remove_name_key(const char* kvs_name, const char* kvs_key) override;

    size_t kvs_get_value_by_name_key(const char* kvs_name,
                                     const char* kvs_key,
                                     char* kvs_val) override;

    size_t kvs_init(const char* main_addr) override;

    size_t kvs_main_server_address_reserve(char* main_addr) override;

    size_t kvs_get_count_names(const char* kvs_name) override;

    size_t kvs_finalize(void) override;

    size_t kvs_get_keys_values_by_name(const char* kvs_name,
                                       char*** kvs_keys,
                                       char*** kvs_values) override;

    size_t kvs_get_replica_size(void) override;

private:
    std::shared_ptr<ccl::kvs_interface> kvs;
};
