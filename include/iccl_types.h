#pragma once

#include "stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* All symbols shall be internal unless marked as ICCL_API */
#ifdef __linux__
#   if __GNUC__ >= 4
#       define ICCL_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
#   else
#       define ICCL_HELPER_DLL_EXPORT
#   endif
#else
#error "unexpected OS"
#endif

#define ICCL_API ICCL_HELPER_DLL_EXPORT

/** Status values returned by ICCL functions. */
typedef enum
{
    iccl_status_success           = 0,
    iccl_status_out_of_resource   = 1,
    iccl_status_invalid_arguments = 2,
    iccl_status_unimplemented     = 3,
    iccl_status_runtime_error     = 4
} iccl_status_t;

/** Data type specification */
typedef enum
{
    iccl_dtype_char   = 0,
    iccl_dtype_int    = 1,
    iccl_dtype_bfp16  = 2,
    iccl_dtype_float  = 3,
    iccl_dtype_double = 4,
    iccl_dtype_int64  = 5,
    iccl_dtype_uint64 = 6,
    iccl_dtype_custom = 7
} iccl_datatype_t;

/** Reduction specification */
typedef enum
{
    iccl_reduction_sum    = 0,
    iccl_reduction_prod   = 1,
    iccl_reduction_min    = 2,
    iccl_reduction_max    = 3,
    iccl_reduction_custom = 4
} iccl_reduction_t;

/* in_buf, in_count, in_dtype, out_buf, out_count, out_dtype, out_dtype_size */
typedef iccl_status_t(*iccl_prologue_fn_t) (const void*, size_t, iccl_datatype_t, void**, size_t*, iccl_datatype_t*, size_t*);

/* in_buf, in_count, in_dtype, out_buf, out_count, out_dtype */
typedef iccl_status_t(*iccl_epilogue_fn_t) (const void*, size_t, iccl_datatype_t, void*, size_t*, iccl_datatype_t);

/* in_buf, in_count, inout_buf, out_count, context, datatype */
typedef iccl_status_t(*iccl_reduction_fn_t) (const void*, size_t, void*, size_t*, const void**, iccl_datatype_t);

/* Extendable list of collective attributes */
typedef struct
{
    iccl_prologue_fn_t prologue_fn;
    iccl_epilogue_fn_t epilogue_fn;
    iccl_reduction_fn_t reduction_fn;
    size_t priority;
    int synchronous;
    int to_cache;
    /**
     * Id of the operation. If specified, new communicator will be created and collective
     * operations with the same @b match_id will be executed in the same order.
     */
    const char* match_id;
} iccl_coll_attr_t;

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
} iccl_comm_attr_t;

typedef void* iccl_comm_t;

typedef void* iccl_request_t;


#ifdef __cplusplus
}   /*extern C */
#endif
