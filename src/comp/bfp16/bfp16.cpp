#include "comp/bfp16/bfp16.hpp"
#include "comp/bfp16/bfp16_intrisics.h"
#include "common/log/log.hpp"

void ccl_bf16_reduce(const void* in_buf, size_t in_cnt,
                     void* inout_buf, size_t* out_cnt,
                     ccl_reduction_t reduction_op)
{
    LOG_DEBUG("bf16 reduction for %zu elements\n", in_cnt);

    if (out_cnt != nullptr)
    {
        *out_cnt = in_cnt;
    }

    bf16_reduction_func_ptr reduction_op_func = nullptr;
    switch (reduction_op)
    {
        case ccl_reduction_sum:
            reduction_op_func = &sum_wrap;
            break;
        case ccl_reduction_prod:
            reduction_op_func = &prod_wrap;
            break;
        case ccl_reduction_min:
            reduction_op_func = &min_wrap;
            break;
        case ccl_reduction_max:
            reduction_op_func = &max_wrap;
            break;
        default:
            CCL_FATAL("unexpected value ", reduction_op);
    }
    
    int i = 0;
    for (i = 0; i <= (int) in_cnt - 16; i+=16)
    {
        ccl_bf16_reduce_256((uint16_t*)in_buf + i, (uint16_t*)inout_buf + i, reduction_op_func);
    }
    ccl_bf16_reduce_masked((uint16_t*)in_buf + i, (uint16_t*)inout_buf + i, (uint8_t)(in_cnt - i), reduction_op_func);
}
