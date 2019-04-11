#include "comp/comp.hpp"
#include "common/log/log.hpp"

#define MLSL_REDUCE(type)                                               \
    do {                                                                \
        type *in_buf_##type = (type *)in_buf;                           \
        type *inout_buf_##type = (type *)inout_buf;                     \
        switch (reduction) {                                            \
            case mlsl_reduction_sum:                                    \
                for (i = 0; i < in_count; i++) {                        \
                    inout_buf_##type[i] += in_buf_##type[i];            \
                }                                                       \
                break;                                                  \
            case mlsl_reduction_prod:                                   \
                for (i = 0; i < in_count; i++) {                        \
                    inout_buf_##type[i] *= in_buf_##type[i];            \
                }                                                       \
                break;                                                  \
            case mlsl_reduction_min:                                    \
                for (i = 0; i < in_count; i++) {                        \
                    inout_buf_##type[i] = std::min(in_buf_##type[i],    \
                                              inout_buf_##type[i]);     \
                }                                                       \
                break;                                                  \
            case mlsl_reduction_max:                                    \
                for (i = 0; i < in_count; i++) {                        \
                    inout_buf_##type[i] = std::max(in_buf_##type[i],    \
                                              inout_buf_##type[i]);     \
                }                                                       \
                break;                                                  \
            default:                                                    \
                MLSL_FATAL("unexpected value ", reduction);             \
        }                                                               \
    } while (0)

mlsl_status_t mlsl_comp_copy(const void *in_buf, void *out_buf, size_t count, mlsl_datatype_internal_t dtype)
{
    memcpy(out_buf, in_buf, count * mlsl_datatype_get_size(dtype));
    return mlsl_status_success;
}

mlsl_status_t mlsl_comp_reduce(const void *in_buf, size_t in_count, void *inout_buf, size_t *out_count,
                               mlsl_datatype_internal_t dtype, mlsl_reduction_t reduction, mlsl_reduction_fn_t reduction_fn)
{
    if (reduction == mlsl_reduction_custom)
    {
        MLSL_THROW_IF_NOT(reduction_fn, "custom reduction requires user callback");
        reduction_fn(in_buf, in_count, inout_buf, out_count, NULL /* context */, dtype->type);
        return mlsl_status_success;
    }

    size_t i;
    switch (dtype->type) {
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
        default:
            MLSL_FATAL("unexpected value ", dtype->type);
            break;
    }
    return mlsl_status_success;
}

const char *mlsl_reduction_to_str(mlsl_reduction_t type)
{
    switch (type) {
        case mlsl_reduction_sum:
            return "SUM";
        case mlsl_reduction_prod:
            return "PROD";
        case mlsl_reduction_min:
            return "MIN";
        case mlsl_reduction_max:
            return "MAX";
        case mlsl_reduction_custom:
            return "CUSTOM";
        default:
            return "UNKNOWN";
    }
}
