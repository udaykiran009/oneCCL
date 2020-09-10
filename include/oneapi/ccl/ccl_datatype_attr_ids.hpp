#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl
{

enum class ccl_datatype_attributes : int {
    version,
    size,

    last_value
};

}
