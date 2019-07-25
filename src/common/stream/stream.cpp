#include "common/stream/stream.hpp"

ccl_stream::ccl_stream(ccl_stream_type_t stream_type,
                         void* native_stream):
    stream_type(stream_type),
    native_stream(native_stream)
{}

