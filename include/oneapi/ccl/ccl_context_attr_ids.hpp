#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {
/**
 * Context attribute ids
 */
enum class context_attr_id : int {
    version,
    cl_backend,
    native_handle,

    last_value
};

} // namespace ccl
