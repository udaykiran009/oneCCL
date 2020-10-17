#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

enum class comm_attr_id : int {
    version,

    last_value
};

} // namespace ccl
