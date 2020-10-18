#include "common/log/log.hpp"
#include "common/device/device.hpp"
#include "oneapi/ccl/native_device_api/export_api.hpp"

ccl_device_impl::ccl_device_impl(device_native_t& dev, const ccl::library_version& version)
    : version(version),
    native_device(dev)
{
}

ccl_device_impl::ccl_device_impl(const device_native_t& dev, const ccl::library_version& version)
    : version(version),
    native_device(dev)
{
}


ccl_device_impl::ccl_device_impl(device_native_t&& dev, const ccl::library_version& version)
    : version(version),
    native_device(std::move(dev))
{
}

ccl_device_impl::ccl_device_impl(device_native_handle_t dev_handle,
                    const ccl::library_version& version)
    : version(version)
{
}

void ccl_device_impl::build_from_params() {
    if (!creation_is_postponed) {
        throw ccl::exception("error");
    }
#ifdef CCL_ENABLE_SYCL
    /* TODO unavailbale??
    event_native_t event_candidate{native_context};
    std::swap(event_candidate, native_event); //TODO USE attributes fro sycl queue construction
    */

    throw ccl::exception("build_from_attr is not availbale for sycl::device");
#else

    //TODO use attributes

#endif
    creation_is_postponed = false;
}

//Export Attributes
typename ccl_device_impl::version_traits_t::type ccl_device_impl::set_attribute_value(
    typename version_traits_t::type val,
    const version_traits_t& t) {
    (void)t;
    throw ccl::exception("Set value for 'ccl::event_attr_id::library_version' is not allowed");
    return version;
}

const typename ccl_device_impl::version_traits_t::return_type& ccl_device_impl::get_attribute_value(
    const version_traits_t& id) const {
    return version;
}

const typename ccl_device_impl::cl_backend_traits_t::return_type& ccl_device_impl::get_attribute_value(
    const cl_backend_traits_t& id) const {

    //TODO
    throw ccl::exception("TODO - Get value for 'ccl::device_attr_id::cl_backend_traits_t' is not inmlemented");
    static constexpr ccl::cl_backend_type ret{ccl::cl_backend_type::empty_backend};
    return ret;
}

typename ccl_device_impl::native_handle_traits_t::return_type& ccl_device_impl::get_attribute_value(
    const native_handle_traits_t& id) {
    return native_device;
}
