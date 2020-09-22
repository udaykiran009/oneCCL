#pragma once

#include "oneapi/ccl/ccl_types.hpp"

#ifdef CCL_BFP16_TARGET_ATTRIBUTES
#ifdef CCL_BFP16_AVX512BF_COMPILER
__attribute__((target("avx512bw,avx512vl,avx512bf16")))
#else
__attribute__((target("avx512bw,avx512vl")))
#endif
void ccl_bfp16_reduce(const void* in_buf, size_t in_cnt,
                     void* inout_buf, size_t* out_cnt,
                     ccl::reduction reduction_op);
#else
void ccl_bfp16_reduce(const void* in_buf,
                      size_t in_cnt,
                      void* inout_buf,
                      size_t* out_cnt,
                      ccl::reduction reduction_op);
#endif

void ccl_convert_fp32_to_bfp16_arrays(void*, void*, size_t);
void ccl_convert_bfp16_to_fp32_arrays(void*, float*, size_t);

#ifdef CCL_BFP16_COMPILER

#ifdef CCL_BFP16_TARGET_ATTRIBUTES
#ifdef CCL_BFP16_AVX512BF_COMPILER
void ccl_convert_fp32_to_bfp16(const void* src, void* dst)
    __attribute__((target("avx512bw,avx512bf16")));
#else
void ccl_convert_fp32_to_bfp16(const void* src, void* dst) __attribute__((target("avx512bw")));
#endif
#endif

#ifdef CCL_BFP16_TARGET_ATTRIBUTES
#ifdef CCL_BFP16_AVX512BF_COMPILER
void ccl_convert_bfp16_to_fp32(const void* src, void* dst)
    __attribute__((target("avx512bw,avx512bf16")));
#else
void ccl_convert_bfp16_to_fp32(const void* src, void* dst) __attribute__((target("avx512bw")));
#endif
#endif

#endif /* CCL_BFP16_COMPILER */