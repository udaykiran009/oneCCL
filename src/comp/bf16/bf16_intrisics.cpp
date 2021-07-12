#include "comp/bf16/bf16_intrisics.hpp"

#ifdef CCL_BF16_COMPILER

BF16_TARGET_ATTRIBUTE_BWF __m512 bf16_sum_wrap(__m512 a, __m512 b) {
    return _mm512_add_ps(a, b);
}

BF16_TARGET_ATTRIBUTE_BWF __m512 bf16_prod_wrap(__m512 a, __m512 b) {
    return _mm512_mul_ps(a, b);
}

BF16_TARGET_ATTRIBUTE_BWF __m512 bf16_min_wrap(__m512 a, __m512 b) {
    return _mm512_min_ps(a, b);
}

BF16_TARGET_ATTRIBUTE_BWF __m512 bf16_max_wrap(__m512 a, __m512 b) {
    return _mm512_max_ps(a, b);
}

BF16_TARGET_ATTRIBUTE_BWF __m512 bf16_reduce(__m512 a, __m512 b, ccl_bf16_reduction_func_ptr op) {
    return (*op)(a, b);
}

#endif // CCL_BF16_COMPILER
