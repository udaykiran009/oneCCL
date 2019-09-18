#include "common/log/log.hpp"
#include "common/stream/stream.hpp"

#ifdef ENABLE_SYCL
#include <CL/sycl.hpp>
#endif /* ENABLE_SYCL */

ccl_stream::ccl_stream(ccl_stream_type_t type,
	                   void* native_stream) :
    type(type),
    native_stream(native_stream)
{
#ifdef ENABLE_SYCL
    if (native_stream && (type == ccl_stream_sycl))
        LOG_INFO("SYCL queue's device is ",
                 static_cast<cl::sycl::queue*>(native_stream)->get_device().get_info<cl::sycl::info::device::name>());
#else
    CCL_THROW_IF_NOT(type != ccl_stream_sycl, "unsupported stream type");
#endif /* ENABLE_SYCL */
}

