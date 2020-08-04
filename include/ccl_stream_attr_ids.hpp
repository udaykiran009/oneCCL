#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

class ccl_stream;
namespace ccl {
/**
 * Stream attribute ids
 */
enum class stream_attr_id : int {
    version,
    native_handle,
    device,
    context,
    ordinal,
    index,
    flags,
    mode,
    priority,

    last_value
};

}
