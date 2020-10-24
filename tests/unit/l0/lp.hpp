#define once

using bf16 = ushort;

#ifdef LIFT_EMBARGO

#include "../../../src/comp/bf16/bf16.hpp"

bf16 fp32_to_bf16(float val) {
  // Host-side conversion with RNE rounding - call routines from bf16.hpp
  void *fp32_val_ptr = reinterpret_cast<void *>(&val);
  ushort res = 0;
  void *bf16_res_ptr = reinterpret_cast<void *>(&res);
  ccl_convert_fp32_to_bf16_arrays(fp32_val_ptr, bf16_res_ptr, 1);
  return res;
}

float bf16_to_fp32(bf16 val) {
  // Host-side conversion - call routines from bf16.hpp
  void *bf16_val_ptr = reinterpret_cast<void *>(&val);
  float res = 0.0;
  float *fp32_res_ptr = &res;
  ccl_convert_bf16_to_fp32_arrays(bf16_val_ptr, fp32_res_ptr, 1);
  return res;
}

#else

float bf16_to_fp32(bf16 val) {
  uint32_t temp = static_cast<uint32_t>(val) << 16;
  return *(reinterpret_cast<float *>(&temp));
}

bf16 fp32_to_bf16(float val) {
  // Truncate
  return (reinterpret_cast<bf16 *>(&val))[1];
}

#endif
