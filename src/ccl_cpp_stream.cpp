#include "stream_impl.hpp"
/*
#include "unified_stream_impl.hpp"
#include "unified_device_impl.hpp"
#include "unified_context_impl.hpp"
*/
namespace ccl
{
CCL_API stream::stream(const typename details::ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::version>::type& version):
        base_t(impl_value_t())
{
}

CCL_API stream::stream(stream&& src) :
        base_t(std::move(src))
{
}

CCL_API stream::stream(impl_value_t&& impl) :
        base_t(std::move(impl))
{
}

CCL_API stream::~stream()
{
}

CCL_API stream::stream(const stream& src) :
        base_t(src)
{
}

CCL_API stream& stream::operator= (const stream&src)
{
    if (src.get_impl() != this->get_impl())
    {
        this->get_impl() = src.get_impl();
    }
    return *this;
}

void stream::build_from_params()
{
    get_impl()->build_from_params();
}
}

#ifdef CCL_ENABLE_SYCL
    API_STREAM_CREATION_FORCE_INSTANTIATION(cl::sycl::queue)
    API_STREAM_CREATION_FORCE_INSTANTIATION(cl_command_queue)
    API_STREAM_CREATION_EXT_FORCE_INSTANTIATION(cl::sycl::device, cl::sycl::context)
#endif

API_STREAM_FORCE_INSTANTIATION(ccl::stream_attr_id::version, ccl_version_t);
API_STREAM_FORCE_INSTANTIATION_GET(ccl::stream_attr_id::native_handle);//, typename ccl::unified_stream_type::ccl_native_t);
API_STREAM_FORCE_INSTANTIATION_GET(ccl::stream_attr_id::device);//, typename ccl::unified_device_type::ccl_native_t);
API_STREAM_FORCE_INSTANTIATION(ccl::stream_attr_id::context, typename ccl::unified_device_context_type::ccl_native_t);
API_STREAM_FORCE_INSTANTIATION(ccl::stream_attr_id::ordinal, uint32_t);
API_STREAM_FORCE_INSTANTIATION(ccl::stream_attr_id::index, uint32_t);
API_STREAM_FORCE_INSTANTIATION(ccl::stream_attr_id::flags, size_t);
API_STREAM_FORCE_INSTANTIATION(ccl::stream_attr_id::mode, size_t);
API_STREAM_FORCE_INSTANTIATION(ccl::stream_attr_id::priority, size_t);
