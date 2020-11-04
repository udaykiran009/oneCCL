#pragma once
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_device_attr_ids.hpp"
#include "oneapi/ccl/ccl_device_attr_ids_traits.hpp"
#include "common/utils/utils.hpp"

class ccl_device_impl {
public:
    using device_native_handle_t = typename ccl::unified_device_type::handle_t;
    using device_native_t = typename ccl::unified_device_type::ccl_native_t;

    ccl_device_impl() = delete;
    ccl_device_impl(const ccl_device_impl& other) = delete;
    ccl_device_impl& operator=(const ccl_device_impl& other) = delete;

    ccl_device_impl(device_native_t& dev, const ccl::library_version& version);
    ccl_device_impl(const device_native_t& dev, const ccl::library_version& version);
    ccl_device_impl(device_native_t&& dev, const ccl::library_version& version);
    ccl_device_impl(device_native_handle_t dev_handle, const ccl::library_version& version);
    ~ccl_device_impl() = default;

    //Export Attributes
    using version_traits_t =
        ccl::detail::ccl_api_type_attr_traits<ccl::device_attr_id, ccl::device_attr_id::version>;
    typename version_traits_t::type set_attribute_value(typename version_traits_t::type val,
                                                        const version_traits_t& t);
    const typename version_traits_t::return_type& get_attribute_value(
        const version_traits_t& id) const;

    using cl_backend_traits_t =
        ccl::detail::ccl_api_type_attr_traits<ccl::device_attr_id, ccl::device_attr_id::cl_backend>;
    const typename cl_backend_traits_t::return_type& get_attribute_value(
        const cl_backend_traits_t& id) const;

    using native_handle_traits_t =
        ccl::detail::ccl_api_type_attr_traits<ccl::device_attr_id,
                                              ccl::device_attr_id::native_handle>;
    typename native_handle_traits_t::return_type& get_attribute_value(
        const native_handle_traits_t& id);

    void build_from_params();

private:
    const ccl::library_version version;
    device_native_t native_device;
    bool creation_is_postponed{ false };
};
