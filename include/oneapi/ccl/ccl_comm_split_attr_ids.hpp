#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

enum class comm_split_attr_id : int {
    version,

    color,
    group,

    last_value
};

enum class
    group_split_type : int { // TODO fill in this enum with the actual values
        undetermined = -1,
        //device,
        thread,
        process,
        //socket,
        //node,
        cluster,

        last_value
    };

} // namespace ccl
