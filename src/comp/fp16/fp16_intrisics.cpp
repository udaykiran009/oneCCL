#include "comp/fp16/fp16_intrisics.hpp"

#ifdef CCL_FP16_COMPILER

FP16_TARGET_ATTRIBUTE_AVX512F __m512 fp16_sum_wrap(__m512 a, __m512 b) {
    return _mm512_add_ps(a, b);
}

FP16_TARGET_ATTRIBUTE_AVX512F __m512 fp16_prod_wrap(__m512 a, __m512 b) {
    return _mm512_mul_ps(a, b);
}

FP16_TARGET_ATTRIBUTE_AVX512F __m512 fp16_min_wrap(__m512 a, __m512 b) {
    return _mm512_min_ps(a, b);
}

FP16_TARGET_ATTRIBUTE_AVX512F __m512 fp16_max_wrap(__m512 a, __m512 b) {
    return _mm512_max_ps(a, b);
}

FP16_TARGET_ATTRIBUTE_AVX512F __m512 fp16_reduce(__m512 a,
                                                 __m512 b,
                                                 ccl_fp16_reduction_func_ptr op) {
    return (*op)(a, b);
}

#endif /* CCL_FP16_COMPILER */
