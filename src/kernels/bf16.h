#pragma once

#ifdef CCL_BF16_GPU_TRUNCATE

float __bf16_to_fp32(ushort V) {
    uint temp = convert_uint(V) << 16;
    return as_float(temp);
}

ushort __fp32_to_bf16(float V) {
    ushort2 temp = as_ushort2(V);
    return temp.s1;
}

#else // CCL_BF16_GPU_TRUNCATE

#ifdef cl_intel_bfloat16_conversions
#pragma OPENCL EXTENSION cl_intel_bfloat16_conversions : enable
#else // cl_intel_bfloat16_conversions

// declare SPIR-V intrinsics directly
ushort __builtin_spirv_OpConvertFToBF16INTEL_f32(float);
float __builtin_spirv_OpConvertBF16ToFINTEL_i16(ushort);

// implement built-in functions using these intrinsics
#define __ovld __attribute__((overloadable))
ushort __ovld intel_convert_bfloat16_as_ushort(float f) {
    return __builtin_spirv_OpConvertFToBF16INTEL_f32(f);
}

float __ovld intel_convert_as_bfloat16_float(ushort u) {
    return __builtin_spirv_OpConvertBF16ToFINTEL_i16(u);
}

#endif // cl_intel_bfloat16_conversions

float __bf16_to_fp32(ushort V) {
    return intel_convert_as_bfloat16_float(V);
}

ushort __fp32_to_bf16(float V) {
    return intel_convert_bfloat16_as_ushort(V);
}

#endif // CCL_BF16_GPU_TRUNCATE

#define DEFINE_BF16SUM_OP(T) \
    T __bf16_sum_##T(T lhs, T rhs) { \
        return __fp32_to_bf16(__bf16_to_fp32(lhs) + __bf16_to_fp32(rhs)); \
    }

#define DEFINE_BF16PROD_OP(T) \
    T __bf16_prod_##T(T lhs, T rhs) { \
        return __fp32_to_bf16(__bf16_to_fp32(lhs) * __bf16_to_fp32(rhs)); \
    }

#define DEFINE_BF16MIN_OP(T) \
    T __bf16_min_##T(T lhs, T rhs) { \
        return __fp32_to_bf16(min(__bf16_to_fp32(lhs), __bf16_to_fp32(rhs))); \
    }

#define DEFINE_BF16MAX_OP(T) \
    T __bf16_max_##T(T lhs, T rhs) { \
        return __fp32_to_bf16(max(__bf16_to_fp32(lhs), __bf16_to_fp32(rhs))); \
    }
