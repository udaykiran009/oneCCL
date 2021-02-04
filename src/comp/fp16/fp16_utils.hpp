#pragma once

#include <stdint.h>
#include <stdio.h>

#ifdef CCL_FP16_COMPILER
#include <immintrin.h>
#endif

typedef enum { ccl_fp16_compiler_none = 0, ccl_fp16_hw_none, ccl_fp16_avx512f } ccl_fp16_impl_type;

__attribute__((__always_inline__)) inline ccl_fp16_impl_type ccl_fp16_get_impl_type() {
#ifdef CCL_FP16_COMPILER

    /* F16C detection */
    // __asm__ __volatile__("cpuid"
    //                          : "=a"(reg[0]), "=b"(reg[1]), "=c"(reg[2]), "=d"(reg[3])
    //                          : "a"(1));
    // is_f16c_enabled = (reg[2] & (1 << 29)) >> 29;

    int is_avx512f_enabled = 0;
    uint32_t reg[4];

    /* baseline AVX512 capabilities used for CCL/FP16 implementation */
    /* CPUID.(EAX=07H, ECX=0):EBX.AVX512F  [bit 16] */
    __asm__ __volatile__("cpuid"
                         : "=a"(reg[0]), "=b"(reg[1]), "=c"(reg[2]), "=d"(reg[3])
                         : "a"(7), "c"(0));
    is_avx512f_enabled = ((reg[1] & (1 << 16)) >> 16);

    if (is_avx512f_enabled)
        return ccl_fp16_avx512f;
    else
        return ccl_fp16_hw_none;

#else
    return ccl_fp16_compiler_none;
#endif
}
