#pragma once

#ifdef CCL_FP16_COMPILER

#include <immintrin.h>
#include <inttypes.h>

#include "comp/fp16/fp16_utils.hpp"

#define CCL_FP16_IN_M256 16

#ifdef CCL_FP16_TARGET_ATTRIBUTES
#define FP16_ALL_ATTRS                "avx512f"
#define FP16_TARGET_ATTRIBUTE_AVX512F __attribute__((target("avx512f")))
#define FP16_TARGET_ATTRIBUTE_ALL     __attribute__((target(FP16_ALL_ATTRS)))
#define FP16_INLINE_TARGET_ATTRIBUTE_AVX512F \
    __attribute__((__always_inline__, target("avx512f"))) inline
#define FP16_INLINE_TARGET_ATTRIBUTE_ALL \
    __attribute__((__always_inline__, target(FP16_ALL_ATTRS))) inline
#else /* CCL_FP16_TARGET_ATTRIBUTES */
#define FP16_TARGET_ATTRIBUTE_AVX512F
#define FP16_TARGET_ATTRIBUTE_ALL
#define FP16_INLINE_TARGET_ATTRIBUTE_AVX512F __attribute__((__always_inline__)) inline
#define FP16_INLINE_TARGET_ATTRIBUTE_ALL     __attribute__((__always_inline__)) inline
#endif /* CCL_FP16_TARGET_ATTRIBUTES */

typedef __m512 (*ccl_fp16_reduction_func_ptr)(__m512 a, __m512 b);
FP16_TARGET_ATTRIBUTE_AVX512F __m512 fp16_sum_wrap(__m512 a, __m512 b);
FP16_TARGET_ATTRIBUTE_AVX512F __m512 fp16_prod_wrap(__m512 a, __m512 b);
FP16_TARGET_ATTRIBUTE_AVX512F __m512 fp16_min_wrap(__m512 a, __m512 b);
FP16_TARGET_ATTRIBUTE_AVX512F __m512 fp16_max_wrap(__m512 a, __m512 b);
FP16_TARGET_ATTRIBUTE_AVX512F __m512 fp16_reduce(__m512 a,
                                                 __m512 b,
                                                 ccl_fp16_reduction_func_ptr op);

FP16_INLINE_TARGET_ATTRIBUTE_AVX512F void ccl_fp32_store_as_fp16(const __m512* src, __m256i* dst) {
    _mm256_storeu_si256(dst, _mm512_cvtps_ph(*src, 0));
}

#define CCL_FP16_REDUCE_FUNC_DEFINITIONS(impl_type) \
\
    FP16_INLINE_TARGET_ATTRIBUTE_ALL void ccl_fp16_reduce_inputs_##impl_type( \
        const void* a, const void* b, void* res, ccl_fp16_reduction_func_ptr op) { \
        __m512 vfp32_in, vfp32_inout; \
        vfp32_in = (__m512)(_mm512_cvtph_ps(_mm256_loadu_si256((__m256i*)a))); \
        vfp32_inout = (__m512)(_mm512_cvtph_ps(_mm256_loadu_si256((__m256i*)b))); \
        __m512 vfp32_out = fp16_reduce(vfp32_in, vfp32_inout, op); \
        _mm256_storeu_si256((__m256i*)(res), _mm512_cvtps_ph(vfp32_out, 0)); \
    } \
\
    FP16_INLINE_TARGET_ATTRIBUTE_ALL void ccl_fp16_reduce_main_##impl_type( \
        const void* in, const void* inout, ccl_fp16_reduction_func_ptr op) { \
        ccl_fp16_reduce_inputs_##impl_type(in, inout, (void*)inout, op); \
    } \
\
    FP16_INLINE_TARGET_ATTRIBUTE_ALL void ccl_fp16_reduce_tile_##impl_type( \
        const void* in, void* inout, uint8_t len, ccl_fp16_reduction_func_ptr op) { \
        if (len == 0) \
            return; \
        uint16_t fp16_res[CCL_FP16_IN_M256]; \
        ccl_fp16_reduce_inputs_##impl_type(in, inout, fp16_res, op); \
        uint16_t* inout_ptr = (uint16_t*)inout; \
        for (int i = 0; i < len; i++) { \
            inout_ptr[i] = fp16_res[i]; \
        } \
    } \
\
    FP16_INLINE_TARGET_ATTRIBUTE_ALL void ccl_fp16_reduce_impl_##impl_type( \
        const void* in_buf, void* inout_buf, size_t in_cnt, ccl_fp16_reduction_func_ptr op) { \
        int i = 0; \
        for (i = 0; i <= (int)in_cnt - CCL_FP16_IN_M256; i += CCL_FP16_IN_M256) { \
            ccl_fp16_reduce_main_##impl_type((uint16_t*)in_buf + i, (uint16_t*)inout_buf + i, op); \
        } \
        ccl_fp16_reduce_tile_##impl_type( \
            (uint16_t*)in_buf + i, (uint16_t*)inout_buf + i, (uint8_t)(in_cnt - i), op); \
    }

CCL_FP16_REDUCE_FUNC_DEFINITIONS(avx512f);

FP16_INLINE_TARGET_ATTRIBUTE_ALL void ccl_fp16_reduce_impl(const void* in_buf,
                                                           void* inout_buf,
                                                           size_t in_cnt,
                                                           ccl_fp16_reduction_func_ptr op,
                                                           ccl_fp16_impl_type impl_type) {
    if (impl_type == ccl_fp16_avx512f)
        ccl_fp16_reduce_impl_avx512f(in_buf, inout_buf, in_cnt, op);
}

#endif /* CCL_FP16_COMPILER */
