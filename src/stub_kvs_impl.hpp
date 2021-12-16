#pragma once

#ifdef CCL_ENABLE_STUB_BACKEND

#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/aliases.hpp"

#include "oneapi/ccl/type_traits.hpp"
#include "oneapi/ccl/types_policy.hpp"

#include "oneapi/ccl/kvs_attr_ids.hpp"
#include "oneapi/ccl/kvs_attr_ids_traits.hpp"
#include "oneapi/ccl/kvs_attr.hpp"

#include "common/global/global.hpp"

#include "kvs_impl.hpp"

namespace ccl {

class stub_kvs_impl : public base_kvs_impl {
public:
    stub_kvs_impl();
    stub_kvs_impl(const kvs::address_type& addr);

    int get_id() const;
};

} // namespace ccl

#endif // CCL_ENABLE_STUB_BACKEND
