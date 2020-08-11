#pragma once
#ifdef MULTI_GPU_SUPPORT
#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl_types.h'"
#endif

/** Device topology class. */
typedef enum {
    ring_algo_class = 0,
    a2a_algo_class = 1,

    ccl_topology_class_last_value
} ccl_topology_class_t;

/** Device topology group. */
typedef enum {
    device_group = 0,
    thread_group = 1,
    process_group = 2,

    ccl_topology_group_last_value
} ccl_topology_group_t;

#endif //MULTI_GPU_SUPPORT
