#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

typedef enum {
    color,
    group,      // ccl_group_split_type or ccl_device_group_split_type
    version,
} ccl_comm_split_attributes;



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
enum class ccl_device_group_split_type { // TODO fill in this enum with the actual values in the final
    //device,
    thread,
    process,
    //socket,
    //node,
    cluster,
};

#endif

} // namespace ccl
