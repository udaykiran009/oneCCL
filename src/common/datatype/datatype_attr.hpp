#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_datatype_attr_ids_traits.hpp"

namespace ccl
{

class ccl_datatype_attr_impl
{
public:
    /**
     * `version` operations
     */
    using version_traits_t = details::ccl_api_type_attr_traits<ccl_datatype_attributes, ccl_datatype_attributes::version>;

    const typename version_traits_t::return_type&
        get_attribute_value(const version_traits_t& id) const
    {
        return library_version;
    }

    typename version_traits_t::return_type
        set_attribute_value(typename version_traits_t::type val, const version_traits_t& t)
    {
        (void)t;
        throw ccl_error("Set value for 'ccl::ccl_datatype_attributes::version' is not allowed");
        return library_version;
    }

    /**
     * `size` operations
     */
    using size_traits_t = details::ccl_api_type_attr_traits<ccl_datatype_attributes, ccl_datatype_attributes::size>;

    const typename size_traits_t::return_type&
        get_attribute_value(const size_traits_t& id) const
    {
        return datatype_size;
    }

    typename size_traits_t::return_type
        set_attribute_value(typename size_traits_t::return_type val, const size_traits_t& t)
    {
        if (val <= 0) {
            throw ccl_error("Size value must be greater than 0");
        }
        auto old = datatype_size;
        datatype_size = val;
        return old;
    }

    ccl_datatype_attr_impl(const typename version_traits_t::return_type& version) :
                                    library_version(version)
    {
    }

protected:
    typename version_traits_t::return_type library_version;
    typename size_traits_t::return_type datatype_size = 1;
};

} // namespace ccl
