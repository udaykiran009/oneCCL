#pragma once

#include <immintrin.h>
#include <inttypes.h>

#if defined(__clang__) || defined(__GNUC__) && !defined(__ICC)
    #define TARGET_ATTRIBUTE_F                      __attribute__((target("avx512f")))
    #define TARGET_ATTRIBUTE_BW                     __attribute__((target("avx512bw")))
    #define TARGET_ATTRIBUTE_BWF                    __attribute__((target("avx512bw,avx512f")))
    #define TARGET_ATTRIBUTE_ALL                    __attribute__((target("avx512bw,avx512vl,avx512f")))
    #define INLINE_TARGET_ATTRIBUTE_BW              __attribute__((__always_inline__, target("avx512bw"))) inline
    #define INLINE_TARGET_ATTRIBUTE_ALL             __attribute__((__always_inline__, target("avx512bw,avx512vl,avx512f"))) inline
#else
    #define TARGET_ATTRIBUTE_F
    #define TARGET_ATTRIBUTE_BW
    #define TARGET_ATTRIBUTE_BWF
    #define TARGET_ATTRIBUTE_ALL
    #define INLINE_TARGET_ATTRIBUTE_BW              __attribute__((__always_inline__)) inline
    #define INLINE_TARGET_ATTRIBUTE_ALL             __attribute__((__always_inline__)) inline
#endif

typedef __m512 (*bf16_reduction_func_ptr)(__m512 a, __m512 b);

inline int __attribute__((__always_inline__)) ccl_detect_avx512f()
{
    uint32_t reg[4];
    /* Baseline AVX512 capabilities used for CCL/BFP16 implementation*/
    /* CPUID.(EAX=07H, ECX=0):EBX.AVX512F  [bit 16] */
    /* CPUID.(EAX=07H, ECX=0):EBX.AVX512BW [bit 30] */
    /* CPUID.(EAX=07H, ECX=0):EBX.AVX512VL [bit 31] */

    __asm__ __volatile__ ("cpuid" : "=a" (reg[0]), "=b" (reg[1]), "=c" (reg[2]), "=d" (reg[3])  : "a" (7), "c" (0));
    return (( reg[1] & (1 << 16) ) >> 16) & (( reg[1] & (1 << 30) ) >> 30) & (( reg[1] & (1 << 31) ) >> 31);
}

TARGET_ATTRIBUTE_BWF __m512 sum_wrap(__m512 a, __m512 b)
{
    return _mm512_add_ps(a, b);
}

TARGET_ATTRIBUTE_BWF __m512 prod_wrap(__m512 a, __m512 b)
{
    return _mm512_mul_ps(a, b);
}

TARGET_ATTRIBUTE_BWF __m512 min_wrap(__m512 a, __m512 b)
{
    return _mm512_min_ps(a, b);
}

TARGET_ATTRIBUTE_BWF __m512 max_wrap(__m512 a, __m512 b)
{
    return _mm512_max_ps(a, b);
}

TARGET_ATTRIBUTE_BWF __m512 ccl_m512_reduce(__m512 a, __m512 b, bf16_reduction_func_ptr reduction_op)
{
    return (*reduction_op)(a, b);
}

INLINE_TARGET_ATTRIBUTE_BW void ccl_bf16_load_as_fp32(const void* src, void* dst)
{
    /* TBD: At some point we may want to use more optimized implementation thru AVX512BF */
    __m512i y = _mm512_cvtepu16_epi32(_mm256_loadu_si256((__m256i const*)src));
    _mm512_storeu_si512(dst, _mm512_bslli_epi128(y, 2));
}

INLINE_TARGET_ATTRIBUTE_BW void ccl_fp32_cvt_bf16(const void* src, void* dst)
{
    _mm256_storeu_si256((__m256i*)(dst), _mm512_cvtepi32_epi16(_mm512_bsrli_epi128(_mm512_loadu_si512(src), 2)));
}

INLINE_TARGET_ATTRIBUTE_ALL void ccl_bf16_reduce_inputs(const void* a, const void* b, void* res, bf16_reduction_func_ptr reduction_op)
{
    __m512  vfp32_in, vfp32_inout;
    ccl_bf16_load_as_fp32(a, (void*)&vfp32_in);
    ccl_bf16_load_as_fp32(b, (void*)&vfp32_inout);
    __m512  vfp32_out = ccl_m512_reduce(vfp32_in, vfp32_inout, reduction_op);
    ccl_fp32_cvt_bf16((const void*)&vfp32_out, res);
}

INLINE_TARGET_ATTRIBUTE_ALL void ccl_bf16_reduce_256(const void* in, const void* inout, bf16_reduction_func_ptr reduction_op)
{
    __m256i vbfp16_out;
    ccl_bf16_reduce_inputs(in, inout, (void*)&vbfp16_out, reduction_op);
    _mm256_storeu_si256( (__m256i*)(inout), vbfp16_out );
}

INLINE_TARGET_ATTRIBUTE_ALL void ccl_bf16_reduce_masked(const void* in, void* inout, uint8_t len, bf16_reduction_func_ptr reduction_op)
{
    if (len == 0) return;

    uint16_t mask = ( (uint16_t) 0xFFFF ) >> (16 - len);
    __m256i vbfp16_out;
    ccl_bf16_reduce_inputs(in, inout, (void*)&vbfp16_out, reduction_op);
    _mm256_mask_storeu_epi16(inout, (__mmask16) mask, vbfp16_out);
}
