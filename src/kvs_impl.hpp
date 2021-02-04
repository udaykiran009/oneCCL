#pragma once

#include <cstring>

#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/kvs/internal_kvs.h"
#include "common/log/log.hpp"
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/kvs.hpp"

namespace ccl {

class kvs_impl {
public:
    kvs_impl(const kvs_attr& attr);

    kvs_impl(const kvs::address_type& addr, const kvs_attr& attr);

    kvs::address_type get_addr();

    vector_class<char> get(const string_class& key);

    void set(const string_class& key, const vector_class<char>& data);

    std::shared_ptr<internal_kvs> get() const;

private:
    const std::string prefix = "USER_DATA";
    std::shared_ptr<internal_kvs> inter_kvs;
    kvs::address_type addr;
};

} // namespace ccl
