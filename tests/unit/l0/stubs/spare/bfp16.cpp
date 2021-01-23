#include "comp/bf16/bf16.hpp"
#include "comp/bf16/bf16_intrisics.hpp"

void ccl_bf16_reduce(const void* in_buf,
                     size_t in_cnt,
                     void* inout_buf,
                     size_t* out_cnt,
                     ccl::reduction reduction_op) {}

void ccl_convert_fp32_to_bf16(const void* src, void* dst) {}

void ccl_convert_bf16_to_fp32(const void* src, void* dst) {}

void ccl_convert_fp32_to_bf16_arrays(void* send_buf, void* send_buf_bf16, size_t count) {}

void ccl_convert_bf16_to_fp32_arrays(void* recv_buf_bf16, float* recv_buf, size_t count) {}
