#pragma once

#include <map>
#include <set>
#include <stdint.h>

#ifdef CCL_BF16_COMPILER
#include <immintrin.h>
#endif

typedef enum {
    ccl_bf16_no_compiler_support = 0,
    ccl_bf16_no_hardware_support,
    ccl_bf16_avx512f,
    ccl_bf16_avx512bf
} ccl_bf16_impl_type;

extern std::map<ccl_bf16_impl_type, std::string> bf16_impl_names;
extern std::map<ccl_bf16_impl_type, std::string> bf16_env_impl_names;

__attribute__((__always_inline__)) inline std::set<ccl_bf16_impl_type> ccl_bf16_get_impl_types() {
    std::set<ccl_bf16_impl_type> result;

#ifdef CCL_BF16_COMPILER
    int is_avx512f_enabled = 0;
    int is_avx512bf_enabled = 0;

    uint32_t reg[4];

    /* baseline AVX512 capabilities for BF16 implementation */
    /* CPUID.(EAX=07H, ECX=0):EBX.AVX512F  [bit 16] */
    /* CPUID.(EAX=07H, ECX=0):EBX.AVX512BW [bit 30] */
    /* CPUID.(EAX=07H, ECX=0):EBX.AVX512VL [bit 31] */
    __asm__ __volatile__("cpuid"
                         : "=a"(reg[0]), "=b"(reg[1]), "=c"(reg[2]), "=d"(reg[3])
                         : "a"(7), "c"(0));
    is_avx512f_enabled =
        ((reg[1] & (1u << 16)) >> 16) & ((reg[1] & (1u << 30)) >> 30) & ((reg[1] & (1u << 31)) >> 31);

#ifdef CCL_BF16_AVX512BF_COMPILER
    /* capabilities for optimized BF16/FP32 conversions */
    /* CPUID.(EAX=07H, ECX=1):EAX[bit 05] */
    __asm__ __volatile__("cpuid"
                         : "=a"(reg[0]), "=b"(reg[1]), "=c"(reg[2]), "=d"(reg[3])
                         : "a"(7), "c"(1));
    is_avx512bf_enabled = (reg[0] & (1 << 5)) >> 5;
#endif // CCL_BF16_AVX512BF_COMPILER

    if (is_avx512f_enabled)
        result.insert(ccl_bf16_avx512f);

    if (is_avx512bf_enabled)
        result.insert(ccl_bf16_avx512bf);

    if (!is_avx512f_enabled && !is_avx512bf_enabled)
        result.insert(ccl_bf16_no_hardware_support);
#else
    result.insert(ccl_bf16_no_compiler_support);
#endif

    return result;
}
