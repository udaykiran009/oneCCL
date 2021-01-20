#pragma once

#include "oneapi/ccl/types.hpp"

#ifdef CCL_FP16_TARGET_ATTRIBUTES
__attribute__((target("avx512f"))) void ccl_fp16_reduce(const void* in_buf,
                                                        size_t in_cnt,
                                                        void* inout_buf,
                                                        size_t* out_cnt,
                                                        ccl::reduction reduction_op);
#else
void ccl_fp16_reduce(const void* in_buf,
                     size_t in_cnt,
                     void* inout_buf,
                     size_t* out_cnt,
                     ccl::reduction reduction_op);
#endif
