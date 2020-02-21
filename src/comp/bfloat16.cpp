#include <immintrin.h>
#include <inttypes.h>

#include "comp/bfloat16.hpp"
#include "common/log/log.hpp"

#if defined(__clang__) || defined(__GNUC__) && !defined(__ICC)
inline __m512 ccl_m512_reduce(__m512 a, __m512 b, ccl_reduction_t reduction_op) __attribute__((target("avx512bw")));
#endif
inline __m512 ccl_m512_reduce(__m512 a, __m512 b, ccl_reduction_t reduction_op)
{
    switch (reduction_op)
    {
        case ccl_reduction_sum:
            return _mm512_add_ps(a, b);
        case ccl_reduction_prod:
            return _mm512_mul_ps(a, b);
        case ccl_reduction_min:
            return _mm512_min_ps(a, b);
        case ccl_reduction_max:
            return _mm512_max_ps(a, b);
        default:
            CCL_FATAL("unexpected value ", reduction_op);
    }
}
#if defined(__clang__) || defined(__GNUC__) && !defined(__ICC)
inline void ccl_bf16_load_as_fp32(const void* src, void* dst) __attribute__((target("avx512bw")));
#endif
inline void ccl_bf16_load_as_fp32(const void* src, void* dst)
{
    /* TBD: At some point we may want to use more optimized implementation thru AVX512BF */
    __m512i y = _mm512_cvtepu16_epi32(_mm256_loadu_si256((__m256i const*)src));
    _mm512_storeu_si512(dst, _mm512_bslli_epi128(y, 2));
}
#if defined(__clang__) || defined(__GNUC__) && !defined(__ICC)
inline void ccl_fp32_cvt_bf16(const void* src, void* dst) __attribute__((target("avx512bw")));
#endif
inline void ccl_fp32_cvt_bf16(const void* src, void* dst)
{
    __m512i vbfp16_32 = _mm512_bsrli_epi128(_mm512_loadu_si512(src), 2);
    _mm256_storeu_si256((__m256i*)(dst), _mm512_cvtepi32_epi16(vbfp16_32));
}
#if defined(__clang__) || defined(__GNUC__) && !defined(__ICC)
inline void ccl_bf16_reduce_inputs(const void* a, const void* b, void* res, ccl_reduction_t reduction_op) __attribute__((target("avx512bw")));
#endif
inline void ccl_bf16_reduce_inputs(const void* a, const void* b, void* res, ccl_reduction_t reduction_op)
{
    __m512  vfp32_in, vfp32_inout;
    ccl_bf16_load_as_fp32(a, (void*)&vfp32_in);
    ccl_bf16_load_as_fp32(b, (void*)&vfp32_inout);
    __m512  vfp32_out = ccl_m512_reduce(vfp32_in, vfp32_inout, reduction_op);
    ccl_fp32_cvt_bf16((const void*)&vfp32_out, res);
}
#if defined(__clang__) || defined(__GNUC__) && !defined(__ICC)
inline void ccl_bf16_reduce_256(const void* in, const void* inout, ccl_reduction_t reduction_op) __attribute__((target("avx512bw")));
#endif
inline void ccl_bf16_reduce_256(const void* in, const void* inout, ccl_reduction_t reduction_op)
{
    __m256i vbfp16_out;
    ccl_bf16_reduce_inputs(in, inout, (void*)&vbfp16_out, reduction_op);
    _mm256_storeu_si256( (__m256i*)(inout), vbfp16_out );
}
#if defined(__clang__) || defined(__GNUC__) && !defined(__ICC)
inline void ccl_bf16_reduce_masked(const void* in, void* inout, uint8_t len, ccl_reduction_t reduction_op) __attribute__((target("avx512vl,avx512bw")));
#endif
inline void ccl_bf16_reduce_masked(const void* in, void* inout, uint8_t len, ccl_reduction_t reduction_op)
{
    if (len == 0) return;

    uint16_t mask = ( (uint16_t) 0xFFFF ) >> (16 - len);
    __m256i vbfp16_out;
    ccl_bf16_reduce_inputs(in, inout, (void*)&vbfp16_out, reduction_op);
    _mm256_mask_storeu_epi16(inout, (__mmask16) mask, vbfp16_out);

}

void ccl_bf16_reduce(const void* in_buf, size_t in_cnt,
                     void* inout_buf, size_t* out_cnt,
                     ccl_reduction_t reduction_op)
{
    fprintf(stderr, "bf16 reduction for %zu elements\n", in_cnt);
    if (out_cnt != nullptr)
        *out_cnt = in_cnt;
    int i = 0;

    for (i = 0; i <= (int) in_cnt - 16; i+=16)
    {
        ccl_bf16_reduce_256((uint16_t*)in_buf + i, (uint16_t*)inout_buf + i, reduction_op);
    }
    ccl_bf16_reduce_masked((uint16_t*)in_buf + i, (uint16_t*)inout_buf + i, (uint8_t) (in_cnt - i), reduction_op);
}
