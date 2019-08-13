#include "common/stream/stream.hpp"

ccl_stream::ccl_stream(ccl_stream_type_t type,
	                   void* native_stream) :
    type(type),
    native_stream(native_stream)
{}

