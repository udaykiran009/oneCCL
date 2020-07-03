#pragma once

#include "stdint.h"
#include "stdlib.h"
#include "ccl_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Status values returned by CCL functions. */
typedef enum
{
    ccl_status_success               = 0,
    ccl_status_out_of_resource       = 1,
    ccl_status_invalid_arguments     = 2,
    ccl_status_unimplemented         = 3,
    ccl_status_runtime_error         = 4,
    ccl_status_blocked_due_to_resize = 5,

    ccl_status_last_value
} ccl_status_t;

/** API version description. */
typedef struct
{
    unsigned int major;
    unsigned int minor;
    unsigned int update;
    const char* product_status;
    const char* build_date;
    const char* full;
} ccl_version_t;

/** Datatypes. */
typedef int ccl_datatype_t;

#define ccl_dtype_char       ((ccl_datatype_t)(0))
#define ccl_dtype_int        ((ccl_datatype_t)(1))
#define ccl_dtype_bfp16      ((ccl_datatype_t)(2))
#define ccl_dtype_float      ((ccl_datatype_t)(3))
#define ccl_dtype_double     ((ccl_datatype_t)(4))
#define ccl_dtype_int64      ((ccl_datatype_t)(5))
#define ccl_dtype_uint64     ((ccl_datatype_t)(6))
#define ccl_dtype_last_value ((ccl_datatype_t)(7))

/** Reduction operations. */
typedef enum
{
    ccl_reduction_sum    = 0,
    ccl_reduction_prod   = 1,
    ccl_reduction_min    = 2,
    ccl_reduction_max    = 3,
    ccl_reduction_custom = 4,

    ccl_reduction_last_value
} ccl_reduction_t;

/** Stream types. */
typedef enum
{
    ccl_stream_host  = 0,
    ccl_stream_cpu   = 1,
    ccl_stream_gpu   = 2,

    ccl_stream_last_value
} ccl_stream_type_t;

/** Resize action types. */
typedef enum ccl_resize_action
{
    /* Wait additional changes for number of ranks */
    ccl_ra_wait     = 0,
    /* Run with current number of ranks */
    ccl_ra_run      = 1,
    /* Finalize work */
    ccl_ra_finalize = 2,
} ccl_resize_action_t;

typedef struct
{
    const char* match_id;
    const size_t offset;
} ccl_fn_context_t;

/* Sparse coalesce modes */
/* Use this variable to set sparse_allreduce coalescing mode:
   ccl_sparse_coalesce_regular run regular coalesce funtion;
   ccl_sparse_coalesce_disable disables coalesce function in sparse_allreduce,
                               allgathered data is returned;
   ccl_sparse_coalesce_keep_precision on every local reduce bfp16 data is
                               converted to fp32, reduced and then converted
                               back to bfp16.
*/
typedef enum ccl_sparse_coalesce_mode
{
    ccl_sparse_coalesce_regular        = 0,
    ccl_sparse_coalesce_disable        = 1,
    ccl_sparse_coalesce_keep_precision = 2
} ccl_sparse_coalesce_mode_t;

/* comm_size */
typedef ccl_resize_action_t(*ccl_resize_fn_t)(size_t comm_size);

/* in_buf, in_count, in_dtype, out_buf, out_count, out_dtype, context */
typedef ccl_status_t(*ccl_prologue_fn_t) (const void*, size_t, ccl_datatype_t,
                                          void**, size_t*, ccl_datatype_t*,
                                          const ccl_fn_context_t*);

/* in_buf, in_count, in_dtype, out_buf, out_count, out_dtype, context */
typedef ccl_status_t(*ccl_epilogue_fn_t) (const void*, size_t, ccl_datatype_t,
                                          void*, size_t*, ccl_datatype_t,
                                          const ccl_fn_context_t*);

/* in_buf, in_count, inout_buf, out_count, dtype, context */
typedef ccl_status_t(*ccl_reduction_fn_t) (const void*, size_t,
                                           void*, size_t*,
                                           ccl_datatype_t,
                                           const ccl_fn_context_t*);

/* idx_buf, idx_count, idx_dtype, val_buf, val_count, val_dtype, fn_context */
typedef ccl_status_t(*ccl_sparse_allreduce_completion_fn_t) (const void*, size_t, ccl_datatype_t,
                                                             const void*, size_t, ccl_datatype_t,
                                                             const void*);

/* idx_count, idx_dtype, val_count, val_dtype, fn_context, out_idx_buf, out_val_buf */
typedef ccl_status_t(*ccl_sparse_allreduce_alloc_fn_t) (size_t, ccl_datatype_t,
                                                        size_t, ccl_datatype_t,
                                                        const void*,
                                                        void**, void**);

/** Extendable list of collective attributes. */
typedef struct
{
    /**
     * Callbacks into application code
     * for pre-/post-processing data
     * and custom reduction operation
     */
    ccl_prologue_fn_t prologue_fn;
    ccl_epilogue_fn_t epilogue_fn;
    ccl_reduction_fn_t reduction_fn;

    /* Priority for collective operation */
    size_t priority;

    /* Blocking/non-blocking */
    int synchronous;

    /* Persistent/non-persistent */
    int to_cache;

    /* Treat buffer as vector/regular - applicable for allgatherv only */
    int vector_buf;

    /**
     * Id of the operation. If specified, new communicator will be created and collective
     * operations with the same @b match_id will be executed in the same order.
     */
    const char* match_id;

    /* Sparse allreduce specific */
    ccl_sparse_allreduce_completion_fn_t sparse_allreduce_completion_fn;
    ccl_sparse_allreduce_alloc_fn_t sparse_allreduce_alloc_fn;
    const void* sparse_allreduce_fn_ctx;
    ccl_sparse_coalesce_mode_t sparse_coalesce_mode;

} ccl_coll_attr_t;

/** List of host communicator attributes. */
typedef struct
{
    /**
     * Used to split global communicator into parts. Ranks with identical color
     * will form a new communicator.
     */
    int color;
    /* List of rank ids for current process. Unused */
    size_t* ranks;
    /* Total number of ranks in the communicator. Unused */
    size_t size;
    /* List of device ids for current process. Unused */
    const size_t* dev_list;
    /* Hint that operation is local to process. Unused */
    int local;
} ccl_comm_attr_t;

typedef struct
{
    ccl_comm_attr_t comm_attr;
    int version;
} ccl_comm_attr_versioned_t;

/** Host attributes
 *
 */
typedef enum
{
    ccl_host_color,
    ccl_host_version

} ccl_comm_split_attributes;

typedef ccl_comm_attr_versioned_t ccl_host_comm_attr_t;

typedef struct
{
    /* Size of single element */
    size_t size;
} ccl_datatype_attr_t;

typedef void* ccl_comm_t;

typedef void* ccl_request_t;

typedef void* ccl_stream_t;

#ifdef MULTI_GPU_SUPPORT
    #include "ccl_device_types.h"
#endif
#ifdef __cplusplus
}   /*extern C */
#endif
