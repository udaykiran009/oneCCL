#pragma once

#include <cstring>
#include <string>

namespace ccl {

namespace preview {

// struct float16 {
//     constexpr float16() : data(0) {}
//     constexpr float16(uint16_t v) : data(v) {}
//     uint16_t data;

//     friend std::ostream& operator<<(std::ostream& out, const float16& v) {
//         out << v.data;
//         return out;
//     }

//     friend bool operator==(const float16& v1, const float16& v2) {
//         return (v1.data == v2.data) ? true : false;
//     }

//     friend bool operator!=(const float16& v1, const float16& v2) {
//         return !(v1 == v2);
//     }

// } __attribute__((packed));

} // namespace preview

namespace v1 {

struct bfloat16;
bfloat16 fp32_to_bf16(float val);
float bf16_to_fp32(bfloat16 val);

struct bfloat16 {
    constexpr bfloat16() : data(0) {}
    constexpr bfloat16(uint16_t v) : data(v) {}
    uint16_t data;

    friend bool operator==(const bfloat16& v1, const bfloat16& v2) {
        return (v1.data == v2.data) ? true : false;
    }

    friend bool operator!=(const bfloat16& v1, const bfloat16& v2) {
        return !(v1 == v2);
    }

#ifdef CCL_GPU_BF16_TRUNCATE

    friend bfloat16 fp32_to_bf16(float val) {
        // Truncate
        uint16_t int_val = 0;
        memcpy(&int_val, reinterpret_cast<uint8_t*>(&val) + 2, 2);
        return bfloat16(int_val);
    }

    friend float bf16_to_fp32(bfloat16 val) {
        float ret = 0;
        uint32_t temp = static_cast<uint32_t>(val.data) << 16;
        memcpy(&ret, &temp, sizeof(temp));
        return ret;
    }

#else

#include "../../../src/comp/bf16/bf16.hpp"

    friend bfloat16 fp32_to_bf16(float val) {
        // Host-side conversion with RNE rounding - call routines from bf16.hpp
        void *fp32_val_ptr = reinterpret_cast<void *>(&val);
        bfloat16 res(0);
        void *bf16_res_ptr = reinterpret_cast<void *>(&res.data);
        ccl_convert_fp32_to_bf16_arrays(fp32_val_ptr, bf16_res_ptr, 1);
        return res;
    }

    friend float bf16_to_fp32(bfloat16 val) {
        // Host-side conversion - call routines from bf16.hpp
        void *bf16_val_ptr = reinterpret_cast<void *>(&val.data);
        float res = 0.0;
        float *fp32_res_ptr = &res;
        ccl_convert_bf16_to_fp32_arrays(bf16_val_ptr, fp32_res_ptr, 1);
        return res;
    }

#endif // CCL_GPU_BF16_TRUNCATE

    friend std::ostream& operator<<(std::ostream& out, const bfloat16& v) {
        out << bf16_to_fp32(v);
        return out;
    }

} __attribute__((packed));

} // namespace v1

using v1::bfloat16;
using v1::fp32_to_bf16;
using v1::bf16_to_fp32;

} // namespace ccl
