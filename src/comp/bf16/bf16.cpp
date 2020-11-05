#include "oneapi/ccl/types.hpp"
#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "comp/bf16/bf16.hpp"
#include "comp/bf16/bf16_intrisics.hpp"
#include "common/utils/enums.hpp"

#define CCL_FLOATS_IN_M512 16
#define CCL_BF16_SHIFT     16

#ifdef CCL_BF16_COMPILER

void ccl_bf16_reduce(const void* in_buf,
                     size_t in_cnt,
                     void* inout_buf,
                     size_t* out_cnt,
                     ccl::reduction reduction_op) {
    LOG_DEBUG("BF16 reduction for %zu elements\n", in_cnt);

    if (out_cnt != nullptr) {
        *out_cnt = in_cnt;
    }

    ccl_bf16_reduction_func_ptr op = nullptr;
    switch (reduction_op) {
        case ccl::reduction::sum: op = &sum_wrap; break;
        case ccl::reduction::prod: op = &prod_wrap; break;
        case ccl::reduction::min: op = &min_wrap; break;
        case ccl::reduction::max: op = &max_wrap; break;
        default: CCL_FATAL("unexpected value ", utils::enum_to_underlying(reduction_op));
    }

    ccl_bf16_reduce_impl(in_buf, inout_buf, in_cnt, op, ccl::global_data::get().bf16_impl_type);
}

void ccl_convert_fp32_to_bf16(const void* src, void* dst) {
#ifdef CCL_BF16_AVX512BF_COMPILER
    if (ccl::global_data::get().bf16_impl_type == ccl_bf16_avx512bf) {
        _mm256_storeu_si256((__m256i*)(dst), _mm512_cvtneps_pbh(_mm512_loadu_ps(src)));
    }
    else
#endif
    {
        _mm256_storeu_si256((__m256i*)(dst),
                            _mm512_cvtepi32_epi16(_mm512_bsrli_epi128(_mm512_loadu_si512(src), 2)));
    }
}

void ccl_convert_bf16_to_fp32(const void* src, void* dst) {
    _mm512_storeu_si512(
        dst,
        _mm512_bslli_epi128(_mm512_cvtepu16_epi32(_mm256_loadu_si256((__m256i const*)src)), 2));
}

void ccl_convert_fp32_to_bf16_arrays(void* send_buf, void* send_buf_bf16, size_t count) {
    int int_val = 0, int_val_shifted = 0;
    float* send_buf_float = (float*)send_buf;
    size_t limit = (count / CCL_FLOATS_IN_M512) * CCL_FLOATS_IN_M512;

    for (size_t i = 0; i < limit; i += CCL_FLOATS_IN_M512) {
        ccl_convert_fp32_to_bf16(send_buf_float + i, ((unsigned char*)send_buf_bf16) + (2 * i));
    }

    /* proceed remaining float's in buffer */
    for (size_t i = limit; i < count; i++) {
        /* iterate over send_buf_bf16 */
        int* send_bfp_tail = (int*)(((char*)send_buf_bf16) + (2 * i));
        /* copy float (4 bytes) data as is to int variable, */
        memcpy(&int_val, &send_buf_float[i], 4);
        /* then perform shift and */
        int_val_shifted = int_val >> CCL_BF16_SHIFT;
        /* save pointer to result */
        *send_bfp_tail = int_val_shifted;
    }
}

void ccl_convert_bf16_to_fp32_arrays(void* recv_buf_bf16, float* recv_buf, size_t count) {
    int int_val = 0, int_val_shifted = 0;
    size_t limit = (count / CCL_FLOATS_IN_M512) * CCL_FLOATS_IN_M512;

    for (size_t i = 0; i < limit; i += CCL_FLOATS_IN_M512) {
        ccl_convert_bf16_to_fp32((char*)recv_buf_bf16 + (2 * i), recv_buf + i);
    }

    /* proceed remaining bf16's in buffer */
    for (size_t i = limit; i < count; i++) {
        /* iterate over recv_buf_bf16 */
        int* recv_bfp_tail = (int*)((char*)recv_buf_bf16 + (2 * i));
        /* copy bf16 data as is to int variable, */
        memcpy(&int_val, recv_bfp_tail, 4);
        /* then perform shift and */
        int_val_shifted = int_val << CCL_BF16_SHIFT;
        /* copy result to output */
        memcpy((recv_buf + i), &int_val_shifted, 4);
    }
}

#else /* CCL_BF16_COMPILER */

void ccl_bf16_reduce(const void* in_buf,
                     size_t in_cnt,
                     void* inout_buf,
                     size_t* out_cnt,
                     ccl::reduction reduction_op) {
    CCL_FATAL("BF16 reduction is requested but CCL was compiled w/o BF16 support");
}

void ccl_convert_fp32_to_bf16_arrays(void* send_buf, void* send_buf_bf16, size_t count) {
    CCL_FATAL("BF16 reduction is requested but CCL was compiled w/o BF16 support");
}

void ccl_convert_bf16_to_fp32_arrays(void* recv_buf_bf16, float* recv_buf, size_t count) {
    CCL_FATAL("BF16 reduction is requested but CCL was compiled w/o BF16 support");
}

#endif /* CCL_BF16_COMPILER */
