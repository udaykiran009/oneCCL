#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "comp/bfp16/bfp16.hpp"
#include "comp/bfp16/bfp16_intrisics.h"

#ifdef CCL_BFP16_COMPILER

void ccl_bfp16_reduce(const void* in_buf, size_t in_cnt,
                      void* inout_buf, size_t* out_cnt,
                      ccl_reduction_t reduction_op)
{
    LOG_DEBUG("BFP16 reduction for %zu elements\n", in_cnt);

    if (out_cnt != nullptr)
    {
        *out_cnt = in_cnt;
    }

    ccl_bfp16_reduction_func_ptr op = nullptr;
    switch (reduction_op)
    {
        case ccl_reduction_sum:
            op = &sum_wrap;
            break;
        case ccl_reduction_prod:
            op = &prod_wrap;
            break;
        case ccl_reduction_min:
            op = &min_wrap;
            break;
        case ccl_reduction_max:
            op = &max_wrap;
            break;
        default:
            CCL_FATAL("unexpected value ", reduction_op);
    }

    ccl_bfp16_reduce_impl(in_buf, inout_buf, in_cnt,
                          op, global_data.bfp16_impl_type);
}

#else /* CCL_BFP16_COMPILER */

void ccl_bfp16_reduce(const void* in_buf, size_t in_cnt,
                      void* inout_buf, size_t* out_cnt,
                      ccl_reduction_t reduction_op)
{
    CCL_FATAL("BFP16 reduction is requested but CCL was compiled w/o BFP16 support");
}

#endif /* CCL_BFP16_COMPILER */
