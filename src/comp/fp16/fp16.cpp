#include "oneapi/ccl/types.hpp"
#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "comp/fp16/fp16.hpp"
#include "comp/fp16/fp16_intrisics.hpp"
#include "common/utils/enums.hpp"

#define CCL_FLOATS_IN_M512 16

#ifdef CCL_FP16_COMPILER

void ccl_fp16_reduce(const void* in_buf,
                     size_t in_cnt,
                     void* inout_buf,
                     size_t* out_cnt,
                     ccl::reduction reduction_op) {
    LOG_DEBUG("FP16 reduction for %zu elements\n", in_cnt);

    if (out_cnt != nullptr) {
        *out_cnt = in_cnt;
    }

    ccl_fp16_reduction_func_ptr op = nullptr;
    switch (reduction_op) {
        case ccl::reduction::sum: op = &fp16_sum_wrap; break;
        case ccl::reduction::prod: op = &fp16_prod_wrap; break;
        case ccl::reduction::min: op = &fp16_min_wrap; break;
        case ccl::reduction::max: op = &fp16_max_wrap; break;
        default: CCL_FATAL("unexpected value ", utils::enum_to_underlying(reduction_op));
    }

    ccl_fp16_reduce_impl(in_buf, inout_buf, in_cnt, op, ccl::global_data::get().fp16_impl_type);
}

#else /* CCL_FP16_COMPILER */

void ccl_fp16_reduce(const void* in_buf,
                     size_t in_cnt,
                     void* inout_buf,
                     size_t* out_cnt,
                     ccl::reduction reduction_op) {
    CCL_FATAL("FP16 reduction is requested but CCL was compiled w/o FP16 support");
}

#endif /* CCL_FP16_COMPILER */
