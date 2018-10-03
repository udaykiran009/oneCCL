#ifndef MLSL_TYPES_H
#define MLSL_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib.h"

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

/** Status values returned by MLSL functions. */
typedef enum {
    mlsl_status_success           = 0,
    mlsl_status_out_of_memory     = 1,
    mlsl_status_invalid_arguments = 2,
    mlsl_status_unimplemented     = 3,
    mlsl_status_runtime_error     = 4
} mlsl_status_t;

/** Data type specification */
typedef enum {
    mlsl_dtype_char   = 0,
    mlsl_dtype_int    = 1,
    mlsl_dtype_bfp16  = 2,
    mlsl_dtype_float  = 3,
    mlsl_dtype_double = 4,
    mlsl_dtype_int64  = 5,
    mlsl_dtype_uint64 = 6
} mlsl_data_type_t;

/** Reduction specification */
typedef enum {
    mlsl_reduction_sum    = 0,
    mlsl_reduction_prod   = 1,
    mlsl_reduction_min    = 2,
    mlsl_reduction_max    = 3,
    mlsl_reduction_custom = 4
} mlsl_reduction_t;


struct mlsl_request;
typedef struct mlsl_request *mlsl_request_t;

/* in_buf, in_count, out_buf, out_count, datatype */
typedef mlsl_status_t(*mlsl_prologue_fn_t) (const void*, size_t, void*, size_t*, mlsl_data_type_t);
typedef mlsl_status_t(*mlsl_epilogue_fn_t) (const void*, size_t, void*, size_t*, mlsl_data_type_t);

/* in_buf, in_count, inout_buf, out_count, context, datatype */
typedef mlsl_status_t(*mlsl_reduction_fn_t) (const void*, size_t, void*, size_t*, void**, mlsl_data_type_t);

/* Extendable list of collective attributes */
struct mlsl_coll_attr
{
    mlsl_prologue_fn_t prologue_fn;
    mlsl_epilogue_fn_t epilogue_fn;
    mlsl_reduction_fn_t reduction_fn;
    size_t priority;
    int synchronous;
    char *match_id;
    int to_cache;
};
typedef struct mlsl_coll_attr mlsl_coll_attr_t;

size_t MLSL_API mlsl_get_dtype_size(mlsl_data_type_t dtype);
mlsl_status_t MLSL_API mlsl_get_priority_range(size_t *min_priority, size_t *max_priority);

#ifdef __cplusplus
}
#endif

#endif /* MLSL_TYPES_H */
