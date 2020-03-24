#pragma once

#include "ccl_types.h"

#ifdef CCL_BFP16_TARGET_ATTRIBUTES
#ifdef CCL_BFP16_AVX512BF_COMPILER
 __attribute__((target("avx512bw,avx512vl,avx512bf16")))
#else
__attribute__((target("avx512bw,avx512vl")))
#endif
void ccl_bfp16_reduce(const void* in_buf, size_t in_cnt,
                     void* inout_buf, size_t* out_cnt,
                     ccl_reduction_t reduction_op);
#else
void ccl_bfp16_reduce(const void* in_buf, size_t in_cnt,
                     void* inout_buf, size_t* out_cnt,
                     ccl_reduction_t reduction_op);
#endif
