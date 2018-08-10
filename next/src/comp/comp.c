#include "comp.h"
#include "log.h"
#include "mlsl_sched.h"
#include "utils.h"

#include <stdint.h>
#include <string.h>

#define MLSL_REDUCE(type)                                           \
    do {                                                            \
        type *in_buf_##type = (type *)in_buf;                       \
        type *inout_buf_##type = (type *)inout_buf;                 \
        switch (reduction) {                                        \
            case mlsl_reduction_sum:                                \
                for (i = 0; i < in_count; i++) {                    \
                    inout_buf_##type[i] += in_buf_##type[i];        \
                }                                                   \
                break;                                              \
            case mlsl_reduction_prod:                               \
                for (i = 0; i < in_count; i++) {                    \
                    inout_buf_##type[i] *= in_buf_##type[i];        \
                }                                                   \
                break;                                              \
            case mlsl_reduction_min:                                \
                for (i = 0; i < in_count; i++) {                    \
                    inout_buf_##type[i] = MIN(in_buf_##type[i],     \
                                              inout_buf_##type[i]); \
                }                                                   \
                break;                                              \
            case mlsl_reduction_max:                                \
                for (i = 0; i < in_count; i++) {                    \
                    inout_buf_##type[i] = MAX(in_buf_##type[i],     \
                                              inout_buf_##type[i]); \
                }                                                   \
                break;                                              \
            default:                                                \
                MLSL_ASSERTP(0);                                    \
        }                                                           \
    } while (0)

mlsl_status_t mlsl_comp_copy(const void *in_buf, void *out_buf, size_t count, mlsl_data_type_t dtype)
{
    memcpy(out_buf, in_buf, count * mlsl_get_dtype_size(dtype));
    return mlsl_status_success;
}

mlsl_status_t mlsl_comp_reduce(const void *in_buf, size_t in_count,
                               void *inout_buf, size_t *out_count,
                               mlsl_data_type_t dtype, mlsl_reduction_t reduction)
{
    size_t i;

    switch (dtype) {
        case mlsl_dtype_char:
            MLSL_REDUCE(char);
            break;
        case mlsl_dtype_int:
            MLSL_REDUCE(int);
            break;
        case mlsl_dtype_bfp16:
            // TODO:
            break;
        case mlsl_dtype_float:
            MLSL_REDUCE(float);
            break;
        case mlsl_dtype_double:
            MLSL_REDUCE(double);
            break;
        case mlsl_dtype_int64:
            MLSL_REDUCE(int64_t);
            break;
        case mlsl_dtype_uint64:
            MLSL_REDUCE(uint64_t);
            break;
    }
    return mlsl_status_success;
}
