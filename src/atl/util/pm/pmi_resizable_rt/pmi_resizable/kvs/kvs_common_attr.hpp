#pragma once

#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_kvs_attr_ids_traits.hpp"

namespace ccl {

class ccl_kvs_attr_impl {
public:
    /**
     * `version` operations
     */
    using version_traits_t = detail::ccl_api_type_attr_traits<kvs_attr_id, kvs_attr_id::version>;

    const typename version_traits_t::return_type& get_attribute_value(
        const version_traits_t& id) const {
        return version;
    }

    typename version_traits_t::return_type set_attribute_value(typename version_traits_t::type val,
                                                               const version_traits_t& t) {
        (void)t;
        throw ccl::exception("Set value for 'ccl::kvs_attr_id::version' is not allowed");
        return version;
    }

    ccl_kvs_attr_impl(const typename version_traits_t::return_type& version) : version(version) {}

    template <kvs_attr_id attr_id>
    bool is_valid() const noexcept {
        return (attr_id == kvs_attr_id::version);
    }

protected:
    typename version_traits_t::return_type version;
};

} // namespace ccl
