#pragma once

#include "stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* All symbols shall be internal unless marked as MLSL_API */
#ifdef __linux__
#   if __GNUC__ >= 4
#       define MLSL_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
#   else
#       define MLSL_HELPER_DLL_EXPORT
#   endif
#else
#error "unexpected OS"
#endif

#define MLSL_API MLSL_HELPER_DLL_EXPORT

/**
 * Maximum length of @b match_id parameter in @b mlsl_coll_attr_t struct
 */
#define MLSL_MATCH_ID_MAX_LEN (256)

/** Status values returned by MLSL functions. */
typedef enum
{
    mlsl_status_success           = 0,
    mlsl_status_out_of_resource   = 1,
    mlsl_status_invalid_arguments = 2,
    mlsl_status_unimplemented     = 3,
    mlsl_status_runtime_error     = 4
} mlsl_status_t;

/** Data type specification */
typedef enum
{
    mlsl_dtype_char   = 0,
    mlsl_dtype_int    = 1,
    mlsl_dtype_bfp16  = 2,
    mlsl_dtype_float  = 3,
    mlsl_dtype_double = 4,
    mlsl_dtype_int64  = 5,
    mlsl_dtype_uint64 = 6,
    mlsl_dtype_custom = 7
} mlsl_datatype_t;

/** Reduction specification */
typedef enum
{
    mlsl_reduction_sum    = 0,
    mlsl_reduction_prod   = 1,
    mlsl_reduction_min    = 2,
    mlsl_reduction_max    = 3,
    mlsl_reduction_custom = 4
} mlsl_reduction_t;

/* in_buf, in_count, in_dtype, out_buf, out_count, out_dtype, out_dtype_size */
typedef mlsl_status_t(*mlsl_prologue_fn_t) (const void*, size_t, mlsl_datatype_t, void**, size_t*, mlsl_datatype_t*, size_t*);

/* in_buf, in_count, in_dtype, out_buf, out_count, out_dtype */
typedef mlsl_status_t(*mlsl_epilogue_fn_t) (const void*, size_t, mlsl_datatype_t, void*, size_t*, mlsl_datatype_t);

/* in_buf, in_count, inout_buf, out_count, context, datatype */
typedef mlsl_status_t(*mlsl_reduction_fn_t) (const void*, size_t, void*, size_t*, const void**, mlsl_datatype_t);

/* Extendable list of collective attributes */
typedef struct
{
    mlsl_prologue_fn_t prologue_fn;
    mlsl_epilogue_fn_t epilogue_fn;
    mlsl_reduction_fn_t reduction_fn;
    size_t priority;
    int synchronous;
    int to_cache;
    /**
     * Id of the operation. If specified, new communicator will be created and collective
     * operations with the same @b match_id will be executed in the same order.
     * Length of the string must not exceed @ref MLSL_MATCH_ID_MAX_LEN
     */
    const char* match_id;
} mlsl_coll_attr_t;

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
} mlsl_comm_attr_t;

typedef void* mlsl_comm_t;

struct mlsl_request;
typedef struct mlsl_request *mlsl_request_t;


#ifdef __cplusplus
}   /*extern C */
#endif
