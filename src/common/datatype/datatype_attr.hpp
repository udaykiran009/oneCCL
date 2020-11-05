#pragma once
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "oneapi/ccl/datatype_attr_ids_traits.hpp"

namespace ccl {

class ccl_datatype_attr_impl {
public:
    /**
     * `version` operations
     */
    using version_traits_t =
        detail::ccl_api_type_attr_traits<datatype_attr_id, datatype_attr_id::version>;

    const typename version_traits_t::return_type& get_attribute_value(
        const version_traits_t& id) const {
        return version;
    }

    typename version_traits_t::return_type set_attribute_value(typename version_traits_t::type val,
                                                               const version_traits_t& t) {
        (void)t;
        throw ccl::exception("Set value for 'ccl::datatype_attr_id::version' is not allowed");
        return version;
    }

    /**
     * `size` operations
     */
    using size_traits_t =
        detail::ccl_api_type_attr_traits<datatype_attr_id, datatype_attr_id::size>;

    const typename size_traits_t::return_type& get_attribute_value(const size_traits_t& id) const {
        return datatype_size;
    }

    typename size_traits_t::return_type set_attribute_value(typename size_traits_t::return_type val,
                                                            const size_traits_t& t) {
        if (val <= 0) {
            throw ccl::exception("Size value must be greater than 0");
        }
        auto old = datatype_size;
        datatype_size = val;
        return old;
    }

    ccl_datatype_attr_impl(const typename version_traits_t::return_type& version)
            : version(version) {}

protected:
    typename version_traits_t::return_type version;
    typename size_traits_t::return_type datatype_size = 1;
};

} // namespace ccl
