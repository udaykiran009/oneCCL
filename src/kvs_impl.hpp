#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_kvs.hpp"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/kvs/internal_kvs.h"

namespace ccl {

class kvs_impl {
public:
    kvs_impl() {
        inter_kvs = std::shared_ptr<internal_kvs>(new internal_kvs());
        inter_kvs->kvs_main_server_address_reserve(addr.data());
        inter_kvs->kvs_init(addr.data());
    }
    kvs_impl(const kvs::address_type& addr) : addr(addr) {
        inter_kvs = std::shared_ptr<internal_kvs>(new internal_kvs());
        inter_kvs->kvs_init(addr.data());
    }
    kvs::address_type get_addr() {
        return addr;
    }

    string_class get(const string_class& prefix, const string_class& key) const {
        char ret[128];
        inter_kvs->kvs_get_value_by_name_key(prefix.c_str(), key.c_str(), ret);
        return string_class(ret);
    }

    void set(const string_class& prefix, const string_class& key, const string_class& data) const {
        inter_kvs->kvs_set_value(prefix.c_str(), key.c_str(), data.c_str());
    }

private:
    std::shared_ptr<internal_kvs> inter_kvs;
    kvs::address_type addr;
};

kvs::address_type CCL_API kvs::get_address() const {
    return pimpl->get_addr();
}

string_class CCL_API kvs::get(const string_class& prefix, const string_class& key) const {
    return pimpl->get(prefix, key);
}

void CCL_API kvs::set(const string_class& prefix,
                      const string_class& key,
                      const string_class& data) const {
    pimpl->set(prefix, key, data);
}

CCL_API kvs::kvs(const kvs::address_type& addr) {
    pimpl = std::unique_ptr<kvs_impl>(new kvs_impl(addr));
}

CCL_API kvs::kvs() {
    pimpl = std::unique_ptr<kvs_impl>(new kvs_impl());
}

CCL_API kvs::~kvs() {}

} // namespace ccl
