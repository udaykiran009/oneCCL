#pragma once

#include <map>
#include <set>
#include <stdint.h>

#ifdef CCL_FP16_COMPILER
#include <immintrin.h>
#endif

typedef enum {
    ccl_fp16_no_compiler_support = 0,
    ccl_fp16_no_hardware_support,
    ccl_fp16_f16c,
    ccl_fp16_avx512f
} ccl_fp16_impl_type;

extern std::map<ccl_fp16_impl_type, std::string> fp16_impl_names;
extern std::map<ccl_fp16_impl_type, std::string> fp16_env_impl_names;

__attribute__((__always_inline__)) inline std::set<ccl_fp16_impl_type> ccl_fp16_get_impl_types() {
    std::set<ccl_fp16_impl_type> result;

#ifdef CCL_FP16_COMPILER
    int is_f16c_enabled = 0;
    int is_avx512f_enabled = 0;

    uint32_t reg[4];

    /* AVX2 capabilities for FP16 implementation */
    __asm__ __volatile__("cpuid" : "=a"(reg[0]), "=b"(reg[1]), "=c"(reg[2]), "=d"(reg[3]) : "a"(1));
    is_f16c_enabled = (reg[2] & (1 << 29)) >> 29;

    /* AVX512 capabilities for FP16 implementation */
    /* CPUID.(EAX=07H, ECX=0):EBX.AVX512F  [bit 16] */
    /* CPUID.(EAX=07H, ECX=0):EBX.AVX512BW [bit 30] */
    /* CPUID.(EAX=07H, ECX=0):EBX.AVX512VL [bit 31] */
    __asm__ __volatile__("cpuid"
                         : "=a"(reg[0]), "=b"(reg[1]), "=c"(reg[2]), "=d"(reg[3])
                         : "a"(7), "c"(0));
    is_avx512f_enabled = ((reg[1] & (1u << 16)) >> 16) & ((reg[1] & (1u << 30)) >> 30) &
                         ((reg[1] & (1u << 31)) >> 31);

    if (is_avx512f_enabled)
        result.insert(ccl_fp16_avx512f);

    if (is_f16c_enabled)
        result.insert(ccl_fp16_f16c);

    if (!is_avx512f_enabled && !is_f16c_enabled)
        result.insert(ccl_fp16_no_hardware_support);
#else
    result.insert(ccl_fp16_no_compiler_support);
#endif

    return result;
}
