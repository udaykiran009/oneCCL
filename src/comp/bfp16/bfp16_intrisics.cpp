//#include "bfp16_intrisics.h"
#include "comp/bfp16/bfp16_intrisics.h"

#ifdef CCL_BFP16_COMPILER
TARGET_ATTRIBUTE_BWF __m512 sum_wrap(__m512 a, __m512 b) {
    return _mm512_add_ps(a, b);
}

TARGET_ATTRIBUTE_BWF __m512 prod_wrap(__m512 a, __m512 b) {
    return _mm512_mul_ps(a, b);
}

TARGET_ATTRIBUTE_BWF __m512 min_wrap(__m512 a, __m512 b) {
    return _mm512_min_ps(a, b);
}

TARGET_ATTRIBUTE_BWF __m512 max_wrap(__m512 a, __m512 b) {
    return _mm512_max_ps(a, b);
}

TARGET_ATTRIBUTE_BWF __m512 ccl_m512_reduce(__m512 a, __m512 b, ccl_bfp16_reduction_func_ptr op) {
    return (*op)(a, b);
}

#endif /* CCL_BFP16_COMPILER */
