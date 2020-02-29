#ifndef BASE_BFP16_HPP
#define BASE_BFP16_HPP

#ifdef CCL_BFP16_COMPILER

#include <immintrin.h>
#include <math.h>

#include "base.hpp"

#define FLOATS_IN_M512  16
#define BFP16_SHIFT     16
#define BFP16_PRECISION 0.0390625 // 2^-8

void print_byte_as_bits(char val)
{
    for (int i = 7; 0 <= i; i--)
    {
        printf("%c", (val & (1 << i)) ? '1' : '0');
    }
}

void print_bits(char * ty, char * val, unsigned char * bytes, size_t num_bytes)
{
    printf("(%*s) %*s = [ ", 15, ty, 16, val);
    for (size_t i = 0; i < num_bytes; i++)
    {
        print_byte_as_bits(bytes[num_bytes - i - 1]);
        printf(" ");
    }
    printf("]\n");
}

#define SHOW(T,V) do { T x = V; print_bits(#T, #V, (unsigned char*) &x, sizeof(x)); } while(0)

// float32 -> bfloat16
#ifdef CCL_BFP16_TARGET_ATTRIBUTES
void convert_fp32_to_bfp16(const void* src, void* dst) __attribute__((target("avx512bw")));
#endif
void convert_fp32_to_bfp16(const void* src, void* dst)
{
    _mm256_storeu_si256((__m256i*)(dst), _mm512_cvtepi32_epi16(_mm512_bsrli_epi128(_mm512_loadu_si512(src), 2))); 
}

// bfloat16 -> float32
#ifdef CCL_BFP16_TARGET_ATTRIBUTES
void convert_bfp16_to_fp32(const void* src, void* dst) __attribute__((target("avx512bw")));
#endif
void convert_bfp16_to_fp32(const void* src, void* dst)
{
    __m512i y = _mm512_cvtepu16_epi32(_mm256_loadu_si256((__m256i const*)src));
    _mm512_storeu_si512(dst, _mm512_bslli_epi128(y, 2));
}

template <typename T>
void convert_fp32_to_bfp16_arrays(T* send_buf, void* send_buf_bfp16, size_t count)
{
    int int_val = 0, int_val_shifted = 0;

    for (int i = 0; i < (count / FLOATS_IN_M512) * FLOATS_IN_M512; i += FLOATS_IN_M512)
    {
        convert_fp32_to_bfp16(send_buf + i, ((char*)send_buf_bfp16)+(2 * i));
    }

    for (int i = (count / FLOATS_IN_M512) * FLOATS_IN_M512; i < count; i ++)
    {
        int* send_bfp_tail = (int*)(((char*)send_buf_bfp16) + (2 * i));
        memcpy(&int_val,&send_buf[i], 4);
        int_val_shifted = int_val >> BFP16_SHIFT;
        *send_bfp_tail = int_val_shifted;
    }
}

template <typename T>
void convert_bfp16_to_fp32_arrays(void* recv_buf_bfp16, T* recv_buf, size_t count)
{
    int int_val = 0, int_val_shifted = 0;

    for (int i = 0; i < (count / FLOATS_IN_M512) * FLOATS_IN_M512; i += FLOATS_IN_M512)
    {
        convert_bfp16_to_fp32((char*)recv_buf_bfp16 + (2 * i), recv_buf + i);
    }

    for (int i = (count / FLOATS_IN_M512) * FLOATS_IN_M512; i < count; i ++)
    {
        float recv_bfp_tail = *(float*)((char*)recv_buf_bfp16 + (2 * i));
        memcpy(&int_val,&recv_bfp_tail,4);
        int_val_shifted = int_val << BFP16_SHIFT;
        memcpy((recv_buf + i), &int_val_shifted, 4);
    }
}

template <typename T>
void make_bfp16_prologue(typed_test_param<T>& param, size_t size)
{
    for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++)
    {
        size_t new_idx = param.buf_indexes[buf_idx];        

        if (param.test_conf.place_type == PT_IN)
        {
            T* recv_buf_orig = param.recv_buf[new_idx].data();
            void* recv_buf_bfp16 = param.recv_buf_bfp16[new_idx].data();
            convert_fp32_to_bfp16_arrays(recv_buf_orig, recv_buf_bfp16, size);
        }
        else
        {
            T* send_buf_orig = param.send_buf[new_idx].data();
            void* send_buf_bfp16 = param.send_buf_bfp16[new_idx].data();
            convert_fp32_to_bfp16_arrays(send_buf_orig, send_buf_bfp16, size);
        }
    }
}

template <typename T>
void make_bfp16_epilogue(typed_test_param<T>& param, size_t size)
{
    for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++)
    {
        size_t new_idx = param.buf_indexes[buf_idx];
        T* recv_buf_orig = param.recv_buf[new_idx].data();  
        void* recv_buf_bfp16 = static_cast<void*>(param.recv_buf_bfp16[new_idx].data());
        convert_bfp16_to_fp32_arrays(recv_buf_bfp16, recv_buf_orig, size);
    }
}

#endif /* CCL_BFP16_COMPILER */

#endif /* BASE_BFP16_HPP */
