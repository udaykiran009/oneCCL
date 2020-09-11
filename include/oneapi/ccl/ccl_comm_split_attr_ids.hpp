#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl
{

enum class ccl_comm_split_attributes : int {
    version,
    color,
    group,      // ccl_group_split_type or device_group_split_type

    last_value
};



/**
 * Host-specific values for the 'group' split attribute
 */
enum class ccl_group_split_type : int {    // TODO fill in this enum with the actual values in the final
    //device,
    thread,
    process,
    //socket,
    //node,
    cluster,

    last_value
};



#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)

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

#endif //#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)

} // namespace ccl
