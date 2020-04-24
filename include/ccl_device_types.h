#pragma once
#ifdef MULTI_GPU_SUPPORT
#ifndef CCL_PRODUCT_FULL
    #error "Do not include this file directly. Please include 'ccl_types.h'"
#endif

/** Device topology class. */
typedef enum
{
    ring_algo_class      = 0,
    a2a_algo_class       = 1,

    ccl_topology_class_last_value
} ccl_topology_class_t;

/** Device topology group. */
typedef enum
{
    device_group         = 0,
    thread_group         = 1,
    process_group        = 2,

    ccl_topology_group_last_value
} ccl_topology_group_t;

/** Device attributes
 *
 */
typedef enum
{
    ccl_device_preferred_topology_class,
    ccl_device_preferred_group

} ccl_device_attributes;

typedef struct
{
    struct device_core_attr
    {
        ccl_comm_attr_t comm_attr;
        int version;
    } core;

    unsigned char data[];
} ccl_comm_device_attr_t;

#endif //MULTI_GPU_SUPPORT
