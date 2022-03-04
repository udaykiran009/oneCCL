#pragma once

#include <unistd.h>
#include <memory>
#include "oneapi/ccl.hpp"
#include "util/pm/pmi_resizable_rt/pmi_resizable/kvs/ikvs_wrapper.h"

class users_kvs final : public ikvs_wrapper {
public:
    users_kvs() = delete;
    users_kvs(std::shared_ptr<ccl::kvs_interface> kvs);

    ~users_kvs() = default;

    kvs_status_t kvs_set_value(const std::string& kvs_name,
                               const std::string& kvs_key,
                               const std::string& kvs_val) override;

    kvs_status_t kvs_remove_name_key(const std::string& kvs_name,
                                     const std::string& kvs_key) override;

    kvs_status_t kvs_get_value_by_name_key(const std::string& kvs_name,
                                           const std::string& kvs_key,
                                           std::string& kvs_val) override;

    kvs_status_t kvs_init(const char* main_addr) override;

    kvs_status_t kvs_main_server_address_reserve(char* main_addr) override;

    kvs_status_t kvs_get_count_names(const std::string& kvs_name, size_t& count_names) override;

    kvs_status_t kvs_finalize(void) override;

    kvs_status_t kvs_get_keys_values_by_name(const std::string& kvs_name,
                                             std::vector<std::string>& kvs_keys,
                                             std::vector<std::string>& kvs_values,
                                             size_t& count) override;

    kvs_status_t kvs_get_replica_size(size_t& replica_size) override;

private:
    std::shared_ptr<ccl::kvs_interface> kvs;
};
