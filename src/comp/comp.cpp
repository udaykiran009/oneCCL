#include "oneapi/ccl/types.hpp"
#include "comp/bf16/bf16.hpp"
#include "comp/comp.hpp"
#include "comp/fp16/fp16.hpp"
#include "common/log/log.hpp"
#include "common/global/global.hpp"
#include "common/utils/enums.hpp"

#define CCL_REDUCE(type) \
    do { \
        type* in_buf_##type = (type*)in_buf; \
        type* inout_buf_##type = (type*)inout_buf; \
        switch (reduction) { \
            case ccl::reduction::sum: \
                for (i = 0; i < in_count; i++) { \
                    inout_buf_##type[i] += in_buf_##type[i]; \
                } \
                break; \
            case ccl::reduction::prod: \
                for (i = 0; i < in_count; i++) { \
                    inout_buf_##type[i] *= in_buf_##type[i]; \
                } \
                break; \
            case ccl::reduction::min: \
                for (i = 0; i < in_count; i++) { \
                    inout_buf_##type[i] = std::min(in_buf_##type[i], inout_buf_##type[i]); \
                } \
                break; \
            case ccl::reduction::max: \
                for (i = 0; i < in_count; i++) { \
                    inout_buf_##type[i] = std::max(in_buf_##type[i], inout_buf_##type[i]); \
                } \
                break; \
            default: CCL_FATAL("unexpected value ", utils::enum_to_underlying(reduction)); \
        } \
    } while (0)

ccl::status ccl_comp_copy(const void* in_buf,
                          void* out_buf,
                          size_t count,
                          const ccl_datatype& dtype) {
    CCL_ASSERT(in_buf, "in_buf is null");
    CCL_ASSERT(out_buf, "out_buf is null");
    CCL_MEMCPY(out_buf, in_buf, count * dtype.size());
    return ccl::status::success;
}

ccl::status ccl_comp_reduce(const void* in_buf,
                            size_t in_count,
                            void* inout_buf,
                            size_t* out_count,
                            const ccl_datatype& dtype,
                            ccl::reduction reduction,
                            ccl::reduction_fn reduction_fn,
                            const ccl::fn_context* context) {
    if (reduction == ccl::reduction::custom) {
        CCL_THROW_IF_NOT(reduction_fn, "custom reduction requires user callback");
        reduction_fn(in_buf, in_count, inout_buf, out_count, dtype.idx(), context);
        return ccl::status::success;
    }

    size_t i;
    switch (dtype.idx()) {
        case ccl::datatype::int8: CCL_REDUCE(int8_t); break;
        case ccl::datatype::uint8: CCL_REDUCE(uint8_t); break;
        case ccl::datatype::int16: CCL_REDUCE(int16_t); break;
        case ccl::datatype::uint16: CCL_REDUCE(uint16_t); break;
        case ccl::datatype::int32: CCL_REDUCE(int32_t); break;
        case ccl::datatype::uint32: CCL_REDUCE(uint32_t); break;
        case ccl::datatype::int64: CCL_REDUCE(int64_t); break;
        case ccl::datatype::uint64: CCL_REDUCE(uint64_t); break;
        case ccl::datatype::float16:
            if (ccl::global_data::get().fp16_impl_type == ccl_fp16_none)
                CCL_FATAL("CCL doesn't support reductions in FP16 on this CPU");
            ccl_fp16_reduce(in_buf, in_count, inout_buf, out_count, reduction);
            break;
        case ccl::datatype::float32: CCL_REDUCE(float); break;
        case ccl::datatype::float64: CCL_REDUCE(double); break;
        case ccl::datatype::bfloat16:
            if (ccl::global_data::get().bf16_impl_type == ccl_bf16_none)
                CCL_FATAL("CCL doesn't support reductions in BF16 on this CPU");
            ccl_bf16_reduce(in_buf, in_count, inout_buf, out_count, reduction);
            break;
        default: CCL_FATAL("unexpected value ", dtype.idx()); break;
    }
    return ccl::status::success;
}

ccl::status ccl_comp_batch_reduce(const void* in_buf,
                                  const std::vector<size_t>& offsets,
                                  size_t in_count,
                                  void* inout_buf,
                                  size_t* out_count,
                                  const ccl_datatype& dtype,
                                  ccl::reduction reduction,
                                  ccl::reduction_fn reduction_fn,
                                  const ccl::fn_context* context,
                                  int bf16_keep_precision_mode,
                                  float* tmp,
                                  float* acc) {
    if (bf16_keep_precision_mode) {
        //->acc, tmp fusion_buffer_cache???

        /* inout_buf => inout_buffer + offsets[0] */
        ccl_convert_bf16_to_fp32_arrays(inout_buf, acc, in_count);

        for (size_t i = 1; i < offsets.size(); i++) {
            ccl_convert_bf16_to_fp32_arrays(
                (char*)in_buf + dtype.size() * offsets[i], tmp, in_count);
            ccl_comp_reduce(tmp,
                            in_count,
                            acc,
                            out_count,
                            ccl::global_data::get().dtypes->get(ccl::datatype::float32),
                            reduction,
                            reduction_fn,
                            context);
        }

        ccl_convert_fp32_to_bf16_arrays(acc, inout_buf, in_count);
    }
    else {
        for (size_t i = 1; i < offsets.size(); i++) {
            ccl_comp_reduce((char*)in_buf + dtype.size() * offsets[i],
                            in_count,
                            inout_buf,
                            out_count,
                            dtype,
                            reduction,
                            reduction_fn,
                            context);
        }
    }

    return ccl::status::success;
}

const char* ccl_reduction_to_str(ccl::reduction type) {
    switch (type) {
        case ccl::reduction::sum: return "SUM";
        case ccl::reduction::prod: return "PROD";
        case ccl::reduction::min: return "MIN";
        case ccl::reduction::max: return "MAX";
        case ccl::reduction::custom: return "CUSTOM";
        default: return "UNKNOWN";
    }
}
