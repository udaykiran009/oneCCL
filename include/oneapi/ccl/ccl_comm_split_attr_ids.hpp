#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

enum class ccl_comm_split_attributes : int {
    color,
    group,      // ccl_group_split_type or device_group_split_type
    version,
};



/**
 * Host-specific values for the 'group' split attribute
 */
enum class ccl_group_split_type {    // TODO fill in this enum with the actual values in the final
    //device,
    thread,
    process,
    //socket,
    //node,
    cluster,
};



#ifdef MULTI_GPU_SUPPORT

/**
 * Device-specific values for the 'group' split attribute
 */
enum class device_group_split_type : int { // TODO fill in this enum with the actual values in the final
    undetermined = -1,
    //device,
    thread,
    process,
    //socket,
    //node,
    cluster,

    last_value
};

#endif

} // namespace ccl
