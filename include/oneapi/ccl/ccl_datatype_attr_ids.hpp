#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl
{

enum class datatype_attr_id : int {
    version,

    size,

    last_value
};

}
