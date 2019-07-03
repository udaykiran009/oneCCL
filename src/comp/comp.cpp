#include "comp/comp.hpp"
#include "common/log/log.hpp"

#define ICCL_REDUCE(type)                                               \
    do {                                                                \
        type *in_buf_##type = (type *)in_buf;                           \
        type *inout_buf_##type = (type *)inout_buf;                     \
        switch (reduction) {                                            \
            case iccl_reduction_sum:                                    \
                for (i = 0; i < in_count; i++) {                        \
                    inout_buf_##type[i] += in_buf_##type[i];            \
                }                                                       \
                break;                                                  \
            case iccl_reduction_prod:                                   \
                for (i = 0; i < in_count; i++) {                        \
                    inout_buf_##type[i] *= in_buf_##type[i];            \
                }                                                       \
                break;                                                  \
            case iccl_reduction_min:                                    \
                for (i = 0; i < in_count; i++) {                        \
                    inout_buf_##type[i] = std::min(in_buf_##type[i],    \
                                              inout_buf_##type[i]);     \
                }                                                       \
                break;                                                  \
            case iccl_reduction_max:                                    \
                for (i = 0; i < in_count; i++) {                        \
                    inout_buf_##type[i] = std::max(in_buf_##type[i],    \
                                              inout_buf_##type[i]);     \
                }                                                       \
                break;                                                  \
            default:                                                    \
                ICCL_FATAL("unexpected value ", reduction);             \
        }                                                               \
    } while (0)

iccl_status_t iccl_comp_copy(const void *in_buf, void *out_buf, size_t count, iccl_datatype_internal_t dtype)
{
    memcpy(out_buf, in_buf, count * iccl_datatype_get_size(dtype));
    return iccl_status_success;
}

iccl_status_t iccl_comp_reduce(const void *in_buf, size_t in_count, void *inout_buf, size_t *out_count,
                               iccl_datatype_internal_t dtype, iccl_reduction_t reduction, iccl_reduction_fn_t reduction_fn)
{
    if (reduction == iccl_reduction_custom)
    {
        ICCL_THROW_IF_NOT(reduction_fn, "custom reduction requires user callback");
        reduction_fn(in_buf, in_count, inout_buf, out_count, NULL /* context */, dtype->type);
        return iccl_status_success;
    }

    size_t i;
    switch (dtype->type) {
        case iccl_dtype_char:
            ICCL_REDUCE(char);
            break;
        case iccl_dtype_int:
            ICCL_REDUCE(int);
            break;
        case iccl_dtype_bfp16:
            // TODO:
            break;
        case iccl_dtype_float:
            ICCL_REDUCE(float);
            break;
        case iccl_dtype_double:
            ICCL_REDUCE(double);
            break;
        case iccl_dtype_int64:
            ICCL_REDUCE(int64_t);
            break;
        case iccl_dtype_uint64:
            ICCL_REDUCE(uint64_t);
            break;
        default:
            ICCL_FATAL("unexpected value ", dtype->type);
            break;
    }
    return iccl_status_success;
}

const char *iccl_reduction_to_str(iccl_reduction_t type)
{
    switch (type) {
        case iccl_reduction_sum:
            return "SUM";
        case iccl_reduction_prod:
            return "PROD";
        case iccl_reduction_min:
            return "MIN";
        case iccl_reduction_max:
            return "MAX";
        case iccl_reduction_custom:
            return "CUSTOM";
        default:
            return "UNKNOWN";
    }
}
