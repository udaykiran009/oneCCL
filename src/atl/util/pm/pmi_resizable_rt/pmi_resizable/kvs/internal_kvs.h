#pragma once

#include <stddef.h>
#include "ikvs_wrapper.h"

class internal_kvs final : public ikvs_wrapper {
public:
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

    ~internal_kvs() override;

    void set_server_address(const std::string& server_addr) {
        server_address = server_addr;
    }

private:
    size_t init_main_server_by_env(void);
    size_t init_main_server_address(const char* main_addr);
    bool is_inited{ false };
    std::string server_address{};
};
