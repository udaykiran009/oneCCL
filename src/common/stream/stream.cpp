#include "common/log/log.hpp"
#include "common/stream/stream.hpp"
#include "common/stream/stream_provider_dispatcher_impl.hpp"
#include "native_device_api/export_api.hpp"

#ifdef CCL_ENABLE_SYCL
    template std::unique_ptr<ccl_stream> stream_provider_dispatcher::create(cl::sycl::queue& native_stream, const ccl_version_t& version);
#endif

#ifdef MULTI_GPU_SUPPORT
    template std::unique_ptr<ccl_stream> stream_provider_dispatcher::create(ze_command_queue_handle_t& native_stream, const ccl_version_t& version);
#else
    template std::unique_ptr<ccl_stream> stream_provider_dispatcher::create(void*& native_stream, const ccl_version_t& version);
#endif


void ccl_stream::build_from_attr()
{
    if (!creation_is_postponed)
    {
        throw ccl::ccl_error("error");
    }
#ifdef CCL_ENABLE_SYCL
    stream_native_t stream_candidate{native_context, native_device};
    std::swap(stream_candidate, native_stream); //TODO USE attributes fro sycl queue construction
#else
    ze_command_queue_desc_t descr = stream_native_device_t::element_type::get_default_queue_desc();

    //TODO use attributes
    native_device->create_cmd_queue(descr);
#endif
    creation_is_postponed = false;
}


//Export Attributes
typename ccl_stream::version_traits_t::type
ccl_stream::set_attribute_value(typename version_traits_t::type val, const version_traits_t& t)
{
    (void)t;
    throw ccl::ccl_error("Set value for 'ccl::stream_attr_id::version' is not allowed");
    return library_version;
}

const typename ccl_stream::version_traits_t::return_type&
ccl_stream::get_attribute_value(const version_traits_t& id) const
{
    return library_version;
}

typename ccl_stream::native_handle_traits_t::return_type
ccl_stream::get_attribute_value(const native_handle_traits_t& id)
{
    if (!native_stream_set)
    {
        throw  ccl::ccl_error("native stream is not set");
    }

    return native_stream;
}


typename ccl_stream::device_traits_t::return_type
ccl_stream::get_attribute_value(const device_traits_t& id)
{
    return native_device;
}


typename ccl_stream::context_traits_t::return_type
ccl_stream::get_attribute_value(const context_traits_t& id)
{
    return native_context;
}

typename ccl_stream::ordinal_traits_t::type
ccl_stream::set_attribute_value(typename ordinal_traits_t::type val, const ordinal_traits_t& t)
{
    auto old = ordinal_val;
    std::swap(ordinal_val, val);
    return old;
}

const typename ccl_stream::ordinal_traits_t::return_type&
ccl_stream::get_attribute_value(const ordinal_traits_t& id) const
{
    return ordinal_val;
}

typename ccl_stream::index_traits_t::type
ccl_stream::set_attribute_value(typename index_traits_t::type val, const index_traits_t& t)
{
    auto old = index_val;
    std::swap(index_val, val);
    return old;
}

const typename ccl_stream::index_traits_t::return_type&
ccl_stream::get_attribute_value(const index_traits_t& id) const
{
    return index_val;
}

typename ccl_stream::flags_traits_t::type
ccl_stream::set_attribute_value(typename flags_traits_t::type val, const flags_traits_t& t)
{
    auto old = flags_val;
    std::swap(flags_val, val);
    return old;
}

const typename ccl_stream::flags_traits_t::return_type&
ccl_stream::get_attribute_value(const flags_traits_t& id) const
{
    return flags_val;
}

typename ccl_stream::mode_traits_t::type
ccl_stream::set_attribute_value(typename mode_traits_t::type val, const mode_traits_t& t)
{
    auto old = mode_val;
    std::swap(mode_val, val);
    return old;
}

const typename ccl_stream::mode_traits_t::return_type&
ccl_stream::get_attribute_value(const mode_traits_t& id) const
{
    return mode_val;
}

typename ccl_stream::priority_traits_t::type
ccl_stream::set_attribute_value(typename priority_traits_t::type val, const priority_traits_t& t)
{
    auto old = priority_val;
    std::swap(priority_val, val);
    return old;
}

const typename ccl_stream::priority_traits_t::return_type&
ccl_stream::get_attribute_value(const priority_traits_t& id) const
{
    return priority_val;
}
