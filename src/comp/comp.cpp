#include "comp/bfp16/bfp16.hpp"
#include "comp/comp.hpp"
#include "common/log/log.hpp"
#include "common/global/global.hpp"
#include "common/utils/utils.hpp"

#define CCL_REDUCE(type)                                                \
    do {                                                                \
        type *in_buf_##type = (type *)in_buf;                           \
        type *inout_buf_##type = (type *)inout_buf;                     \
        switch (reduction) {                                            \
            case ccl_reduction_sum:                                     \
                for (i = 0; i < in_count; i++) {                        \
                    inout_buf_##type[i] += in_buf_##type[i];            \
                }                                                       \
                break;                                                  \
            case ccl_reduction_prod:                                    \
                for (i = 0; i < in_count; i++) {                        \
                    inout_buf_##type[i] *= in_buf_##type[i];            \
                }                                                       \
                break;                                                  \
            case ccl_reduction_min:                                     \
                for (i = 0; i < in_count; i++) {                        \
                    inout_buf_##type[i] = std::min(in_buf_##type[i],    \
                                              inout_buf_##type[i]);     \
                }                                                       \
                break;                                                  \
            case ccl_reduction_max:                                     \
                for (i = 0; i < in_count; i++) {                        \
                    inout_buf_##type[i] = std::max(in_buf_##type[i],    \
                                              inout_buf_##type[i]);     \
                }                                                       \
                break;                                                  \
            default:                                                    \
                CCL_FATAL("unexpected value ", reduction);              \
        }                                                               \
    } while (0)

ccl_status_t ccl_comp_copy(const void* in_buf, void* out_buf, size_t count, const ccl_datatype& dtype)
{
    CCL_ASSERT(in_buf, "in_buf is null");
    CCL_ASSERT(out_buf, "out_buf is null");
    CCL_MEMCPY(out_buf, in_buf, count * dtype.size());
    return ccl_status_success;
}

ccl_status_t ccl_comp_reduce(const void* in_buf, size_t in_count, void* inout_buf, size_t* out_count,
                             const ccl_datatype& dtype, ccl_reduction_t reduction,
                             ccl_reduction_fn_t reduction_fn, const ccl_fn_context_t* context)
{
    if (reduction == ccl_reduction_custom)
    {
        CCL_THROW_IF_NOT(reduction_fn, "custom reduction requires user callback");
        reduction_fn(in_buf, in_count, inout_buf, out_count, dtype.idx(), context);
        return ccl_status_success;
    }

    size_t i;
    switch (dtype.idx())
    {
        case ccl_dtype_char:
            CCL_REDUCE(char);
            break;
        case ccl_dtype_int:
            CCL_REDUCE(int);
            break;
        case ccl_dtype_bfp16:
            if (ccl::global_data::get().bfp16_impl_type == ccl_bfp16_none)
                CCL_FATAL("CCL doesn't support reductions in BFP16 on this CPU");
            ccl_bfp16_reduce(in_buf, in_count, inout_buf, out_count, reduction);
            break;
        case ccl_dtype_float:
            CCL_REDUCE(float);
            break;
        case ccl_dtype_double:
            CCL_REDUCE(double);
            break;
        case ccl_dtype_int64:
            CCL_REDUCE(int64_t);
            break;
        case ccl_dtype_uint64:
            CCL_REDUCE(uint64_t);
            break;
        default:
            CCL_FATAL("unexpected value ", dtype.idx());
            break;
    }
    return ccl_status_success;
}

ccl_status_t ccl_comp_batch_reduce(const void* in_buf, const std::vector<size_t>& offsets, 
                                   size_t in_count, void* inout_buf, size_t* out_count,
                                   const ccl_datatype& dtype, ccl_reduction_t reduction,
                                   ccl_reduction_fn_t reduction_fn, const ccl_fn_context_t* context)
{
    float* acc = (float*)malloc(sizeof(float) * in_count);
    float* tmp = (float*)malloc(sizeof(float) * in_count); //-> fusion_buffer_cache
    
    /* inout_buf => inout_buffer + offsets[0] */
    ccl_convert_bfp16_to_fp32_arrays(inout_buf, acc, in_count);

    for(size_t i = 1; i < offsets.size(); i++)
    {
       ccl_convert_bfp16_to_fp32_arrays((char*)in_buf + dtype.size() * offsets[i], tmp, in_count);
       ccl_comp_reduce(tmp, in_count, acc, out_count,
                       ccl::global_data::get().dtypes->get(ccl_dtype_float),
                       reduction, reduction_fn, context);
    }
    
    ccl_convert_fp32_to_bfp16_arrays(acc, inout_buf, in_count);

    free(acc);
    free(tmp);

    return ccl_status_success;
}

const char *ccl_reduction_to_str(ccl_reduction_t type)
{
    switch (type)
    {
        case ccl_reduction_sum:
            return "SUM";
        case ccl_reduction_prod:
            return "PROD";
        case ccl_reduction_min:
            return "MIN";
        case ccl_reduction_max:
            return "MAX";
        case ccl_reduction_custom:
            return "CUSTOM";
        default:
            return "UNKNOWN";
    }
}
