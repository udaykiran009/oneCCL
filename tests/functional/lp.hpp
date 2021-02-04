#pragma once

#if defined CCL_BF16_COMPILER || defined CCL_FP16_COMPILER
#include <immintrin.h>
#endif
#include <math.h>

#include "base.hpp"
#include "conf.hpp"

template <typename T>
struct test_operation;

#define FLOATS_IN_M512 16
#define FLOATS_IN_M256 8

#define FP16_PRECISION (9.77e-4) // 2^-10
#define FP32_PRECISION (1.19e-7) // 2^-23
#define FP64_PRECISION (2.22e-16) // 2^-52
#define BF16_PRECISION (7.81e-3) // 2^-7

bool is_lp_datatype(ccl_data_type dtype);
int is_fp16_enabled();
int is_bf16_enabled();
int is_avx512bf_enabled();

#ifdef CCL_FP16_TARGET_ATTRIBUTES
void convert_fp32_to_fp16(const void* src, void* dst) __attribute__((target("avx512f")));
#else
void convert_fp32_to_fp16(const void* src, void* dst);
#endif

#ifdef CCL_FP16_TARGET_ATTRIBUTES
void convert_fp16_to_fp32(const void* src, void* dst) __attribute__((target("avx512f")));
#else
void convert_fp16_to_fp32(const void* src, void* dst);
#endif

#ifdef CCL_BF16_TARGET_ATTRIBUTES
#ifdef CCL_BF16_AVX512BF_COMPILER
void convert_fp32_to_bf16(const void* src, void* dst)
    __attribute__((target("avx512bw,avx512bf16")));
#else
void convert_fp32_to_bf16(const void* src, void* dst) __attribute__((target("avx512bw")));
#endif
#else
void convert_fp32_to_bf16(const void* src, void* dst);
#endif

#ifdef CCL_BF16_TARGET_ATTRIBUTES
#ifdef CCL_BF16_AVX512BF_COMPILER
void convert_bf16_to_fp32(const void* src, void* dst)
    __attribute__((target("avx512bw,avx512bf16")));
#else
void convert_bf16_to_fp32(const void* src, void* dst) __attribute__((target("avx512bw")));
#endif
#else
void convert_bf16_to_fp32(const void* src, void* dst);
#endif

void convert_lp_to_fp32(const void* src, void* dst, ccl_data_type dtype);
void convert_fp32_to_lp(const void* src, void* dst, ccl_data_type dtype);

template <typename T>
void convert_fp32_to_lp_arrays(T* buf, short* lp_buf, size_t count, ccl_data_type dtype);

template <typename T>
void convert_lp_to_fp32_arrays(short* lp_buf, T* buf, size_t count, ccl_data_type dtype);

template <typename T>
void make_lp_prologue(test_operation<T>& op, size_t size);

template <typename T>
void make_lp_epilogue(test_operation<T>& op, size_t size);

#include "lp_impl.hpp"
