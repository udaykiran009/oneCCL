#pragma once

#include "stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* All symbols shall be internal unless marked as CCL_API */
#ifdef __linux__
#   if __GNUC__ >= 4
#       define CCL_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
#   else
#       define CCL_HELPER_DLL_EXPORT
#   endif
#else
#error "unexpected OS"
#endif

#define CCL_API CCL_HELPER_DLL_EXPORT

/** Status values returned by CCL functions. */
typedef enum
{
    ccl_status_success           = 0,
    ccl_status_out_of_resource   = 1,
    ccl_status_invalid_arguments = 2,
    ccl_status_unimplemented     = 3,
    ccl_status_runtime_error     = 4
} ccl_status_t;

/** Data type specification */
typedef enum
{
    ccl_dtype_char   = 0,
    ccl_dtype_int    = 1,
    ccl_dtype_bfp16  = 2,
    ccl_dtype_float  = 3,
    ccl_dtype_double = 4,
    ccl_dtype_int64  = 5,
    ccl_dtype_uint64 = 6,
    ccl_dtype_custom = 7
} ccl_datatype_t;

/** Reduction specification */
typedef enum
{
    ccl_reduction_sum    = 0,
    ccl_reduction_prod   = 1,
    ccl_reduction_min    = 2,
    ccl_reduction_max    = 3,
    ccl_reduction_custom = 4
} ccl_reduction_t;

/** Stream types. */
typedef enum
{
    ccl_stream_cpu  = 0,
    ccl_stream_sycl = 1
} ccl_stream_type_t;

/* in_buf, in_count, in_dtype, out_buf, out_count, out_dtype, out_dtype_size */
typedef ccl_status_t(*ccl_prologue_fn_t) (const void*, size_t, ccl_datatype_t, void**, size_t*, ccl_datatype_t*, size_t*);

/* in_buf, in_count, in_dtype, out_buf, out_count, out_dtype */
typedef ccl_status_t(*ccl_epilogue_fn_t) (const void*, size_t, ccl_datatype_t, void*, size_t*, ccl_datatype_t);

/* in_buf, in_count, inout_buf, out_count, context, datatype */
typedef ccl_status_t(*ccl_reduction_fn_t) (const void*, size_t, void*, size_t*, const void**, ccl_datatype_t);

/* Extendable list of collective attributes */
typedef struct
{
    ccl_prologue_fn_t prologue_fn;
    ccl_epilogue_fn_t epilogue_fn;
    ccl_reduction_fn_t reduction_fn;
    size_t priority;
    int synchronous;
    int to_cache;
    /**
     * Id of the operation. If specified, new communicator will be created and collective
     * operations with the same @b match_id will be executed in the same order.
     */
    const char* match_id;
} ccl_coll_attr_t;

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

typedef void* ccl_comm_t;

typedef void* ccl_request_t;

typedef void* ccl_stream_t;

#ifdef __cplusplus
}   /*extern C */
#endif
