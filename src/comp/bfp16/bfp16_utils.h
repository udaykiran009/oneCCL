#pragma once

#include <stdint.h>

typedef enum
{
    ccl_bfp16_none = 0,
    ccl_bfp16_avx512f,
    ccl_bfp16_avx512bf
} ccl_bfp16_impl_type;

__attribute__((__always_inline__)) inline
ccl_bfp16_impl_type ccl_bfp16_get_impl_type()
{
#ifdef CCL_BFP16_COMPILER
    int is_avx512f_enabled = 0;

    uint32_t reg[4];

    /* baseline AVX512 capabilities used for CCL/BFP16 implementation */
    /* CPUID.(EAX=07H, ECX=0):EBX.AVX512F  [bit 16] */
    /* CPUID.(EAX=07H, ECX=0):EBX.AVX512BW [bit 30] */
    /* CPUID.(EAX=07H, ECX=0):EBX.AVX512VL [bit 31] */
    __asm__ __volatile__ ("cpuid" :
                          "=a" (reg[0]), "=b" (reg[1]), "=c" (reg[2]), "=d" (reg[3])  :
                          "a" (7), "c" (0));
    is_avx512f_enabled = (( reg[1] & (1 << 16) ) >> 16) &
                         (( reg[1] & (1 << 30) ) >> 30) &
                         (( reg[1] & (1 << 31) ) >> 31);

#ifdef CCL_BFP16_AVX512BF_COMPILER
    int is_avx512bf_enabled = 0;
    /* capabilities for optimized BFP16/FP32 conversions */
    /* CPUID.(EAX=07H, ECX=1):EAX[bit 05] */
    __asm__ __volatile__ ("cpuid" :
                          "=a" (reg[0]), "=b" (reg[1]), "=c" (reg[2]), "=d" (reg[3]) :
                          "a" (7), "c" (1));
    is_avx512bf_enabled = ( reg[0] & (1 << 5) ) >> 5;

    if (is_avx512bf_enabled)
        return ccl_bfp16_avx512bf;
    else
#endif /* CCL_BFP16_AVX512BF_COMPILER */
    if (is_avx512f_enabled)
        return ccl_bfp16_avx512f;
    else
        return ccl_bfp16_none;
#else
    return ccl_bfp16_none;
#endif
}
