#include "common/log/log.hpp"
#include "common/stream/stream.hpp"

ccl_stream::ccl_stream(ccl_stream_type_t type,
	                   void* native_stream) :
    type(type),
    native_stream(native_stream)
{
#ifndef ENABLE_SYCL
	CCL_THROW_IF_NOT(type != ccl_stream_sycl, "unsupported stream type");
#endif
}

