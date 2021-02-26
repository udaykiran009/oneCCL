#pragma once

#include "oneapi/ccl/types.hpp"

#ifdef CCL_FP16_TARGET_ATTRIBUTES
__attribute__((target("avx512f,f16c"))) void ccl_fp16_reduce(const void* in_buf,
                                                             size_t in_cnt,
                                                             void* inout_buf,
                                                             size_t* out_cnt,
                                                             ccl::reduction reduction_op);
#else /* CCL_FP16_TARGET_ATTRIBUTES */
void ccl_fp16_reduce(const void* in_buf,
                     size_t in_cnt,
                     void* inout_buf,
                     size_t* out_cnt,
                     ccl::reduction reduction_op);
#endif /* CCL_FP16_TARGET_ATTRIBUTES */

#ifdef CCL_FP16_COMPILER
#ifdef CCL_FP16_TARGET_ATTRIBUTES
void ccl_convert_fp32_to_fp16(const void* src, void* dst) __attribute__((target("f16c")));
void ccl_convert_fp16_to_fp32(const void* src, void* dst) __attribute__((target("f16c")));
#endif /* CCL_FP16_TARGET_ATTRIBUTES */
#endif /* CCL_FP16_COMPILER */
