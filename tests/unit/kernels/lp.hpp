#pragma once

#include "oneapi/ccl/lp_types.hpp"

using bfloat16 = ccl::bfloat16;
using float16 = ccl::float16;

#ifdef CCL_BF16_GPU_TRUNCATE

bfloat16 fp32_to_bf16(float val) {
    // Truncate
    uint16_t int_val = 0;
    memcpy(&int_val, reinterpret_cast<uint8_t*>(&val) + 2, 2);
    return bfloat16(int_val);
}

float bf16_to_fp32(bfloat16 val) {
    float ret = 0;
    uint32_t temp = static_cast<uint32_t>(val.get_data()) << 16;
    memcpy(&ret, &temp, sizeof(temp));
    return ret;
}

#else /* CCL_BF16_GPU_TRUNCATE */

#include "comp/bf16/bf16.hpp"

bfloat16 fp32_to_bf16(float val) {
    // Host-side conversion with RNE rounding - call routines from bf16.hpp
    void* fp32_val_ptr = reinterpret_cast<void*>(&val);
    bfloat16 res(0);
    void* bf16_res_ptr = reinterpret_cast<void*>(&res.data);
    ccl_convert_fp32_to_bf16_arrays(fp32_val_ptr, bf16_res_ptr, 1);
    return res;
}

float bf16_to_fp32(bfloat16 val) {
    // Host-side conversion - call routines from bf16.hpp
    uint16_t bf16_val = val.get_data();
    void* bf16_val_ptr = &bf16_val;
    float res = 0.0;
    float* fp32_res_ptr = &res;
    ccl_convert_bf16_to_fp32_arrays(bf16_val_ptr, fp32_res_ptr, 1);
    return res;
}

#endif /* CCL_BF16_GPU_TRUNCATE */

#ifdef CCL_FP16_GPU_TRUNCATE

float16 fp32_to_fp16(float val) {
    uint32_t ans;
    uint32_t* val_ptr = (reinterpret_cast<uint32_t*>(&val));
    uint32_t exp_bits = (*val_ptr & 0x7F800000);
    uint32_t significand_bits = (*val_ptr & 0x007FFFFF);
    if (exp_bits == 0x00000000) {
        ans = (*val_ptr & 0x80000000) >> 16;
    }
    else if (exp_bits == 0x7F800000) {
        if (significand_bits != 0) {
            ans = ((*val_ptr & 0x80000000) >> 16) | 0x00007C01;
        }
        else {
            ans = ((*val_ptr & 0x80000000) >> 16) | 0x00007C00;
        }
    }
    else if (exp_bits < 0x38800000) {
        ans = 0xFC00;
    }
    else if (exp_bits > 0x47000000) {
        ans = 0x7C00;
    }
    else {
        ans = ((*val_ptr & 0x80000000) >> 16) | ((((*val_ptr & 0x7F800000) >> 23) - 112) << 10) |
              ((*val_ptr & 0x007FFFFF) >> 13);
    }
    return float16(ans);
}

float fp16_to_fp32(float16 val) {
    uint16_t val_data = val.get_data();
    float ans = 0.0f;
    uint32_t ans_bits = 0;
    uint32_t exp_bits = val_data & 0x7C00;
    uint32_t significand_bits = val_data & 0x03FF;
    if (exp_bits == 0x7C00) {
        ans_bits = ((val_data & 0x8000) << 16) | 0x7F800000 | (significand_bits << 13);
    }
    else if (exp_bits == 0x0000) {
        if (significand_bits != 0x00000000) {
            ans_bits = ((val_data & 0x8000) << 16);
        }
        else {
            ans_bits = ((val_data & 0x8000) << 16) | (significand_bits << 13);
        }
    }
    else {
        ans_bits =
            ((val_data & 0x8000) << 16) | ((exp_bits + 0x1C000) << 13) | (significand_bits << 13);
    }
    std::memcpy(reinterpret_cast<void*>(&ans), reinterpret_cast<void*>(&ans_bits), 4);
    return ans;
}

#else /* CCL_FP16_GPU_TRUNCATE */

#include "comp/fp16/fp16.hpp"

float16 fp32_to_fp16(float val) {
    float vals_holder[16] = { 0 };
    uint16_t ans_holder[16] = { 0 };
    vals_holder[0] = val;
    ccl_convert_fp32_to_fp16((void*)(&vals_holder), (void*)(&ans_holder));
    return float16(ans_holder[0]);
}

float fp16_to_fp32(float16 val) {
    uint16_t vals_holder[16] = { 0 };
    float ans_holder[16] = { 0 };
    vals_holder[0] = val.get_data();
    ccl_convert_fp16_to_fp32((void*)(&vals_holder), (void*)(&ans_holder));
    return ans_holder[0];
}

#endif /* CCL_FP16_GPU_TRUNCATE */

namespace ccl {
namespace v1 {

std::ostream& operator<<(std::ostream& out, const bfloat16& v) {
    //out << bf16_to_fp32(v);
    out << bf16_to_fp32(v) << "|" << v.get_data();
    return out;
}

std::string to_string(const bfloat16& v) {
    std::stringstream ss;
    ss << bf16_to_fp32(v) << "|" << v.get_data();
    return ss.str();
}

std::ostream& operator<<(std::ostream& out, const float16& v) {
    out << fp16_to_fp32(v) << "|" << v.get_data();
    return out;
}

std::string to_string(const float16& v) {
    std::stringstream ss;
    ss << fp16_to_fp32(v) << "|" << v.get_data();
    return ss.str();
}

} // namespace v1
} // namespace ccl
