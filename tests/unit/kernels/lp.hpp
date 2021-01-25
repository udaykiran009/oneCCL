#define once

#include "oneapi/ccl/lp_types.hpp"

using bfloat16 = ccl::bfloat16;

#ifdef CCL_GPU_BF16_TRUNCATE

bfloat16 fp32_to_bf16(float val) {
    // Truncate
    uint16_t int_val = 0;
    memcpy(&int_val, reinterpret_cast<uint8_t*>(&val) + 2, 2);
    return bfloat16(int_val);
}

float bf16_to_fp32(bfloat16 val) {
    float ret = 0;
    uint32_t temp = static_cast<uint32_t>(val.data) << 16;
    memcpy(&ret, &temp, sizeof(temp));
    return ret;
}

#else

#include "../../../src/comp/bf16/bf16.hpp"

bfloat16 fp32_to_bf16(float val) {
    // Host-side conversion with RNE rounding - call routines from bf16.hpp
    void *fp32_val_ptr = reinterpret_cast<void *>(&val);
    bfloat16 res(0);
    void *bf16_res_ptr = reinterpret_cast<void *>(&res.data);
    ccl_convert_fp32_to_bf16_arrays(fp32_val_ptr, bf16_res_ptr, 1);
    return res;
}

float bf16_to_fp32(bfloat16 val) {
    // Host-side conversion - call routines from bf16.hpp
    void *bf16_val_ptr = reinterpret_cast<void *>(&val.data);
    float res = 0.0;
    float *fp32_res_ptr = &res;
    ccl_convert_bf16_to_fp32_arrays(bf16_val_ptr, fp32_res_ptr, 1);
    return res;
}

#endif

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
} // namespace v1
} // namespace ccl
