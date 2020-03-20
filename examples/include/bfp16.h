#include <immintrin.h>

#define FLOATS_IN_M512  16
#define BFP16_SHIFT     16

/*

 https://www.johndcook.com/blog/2018/11/15/bfloat16/ 

 In this example we use the accuracy 0.00781250
 of calculations performed in the bfloat16, but don't take
 into account the error that may occur during conversion
 from float32 datatype to bfloat16. 
 
 */

#define BFP16_PRECISION 0.00781250 /* 2^-7 = 0.00781250 */

void convert_fp32_to_bfp16_arrays(void*, void*, int);
void convert_bfp16_to_fp32_arrays(void*, float*, int);

int is_bfp16_enabled()
{
#ifdef CCL_BFP16_COMPILER
    int is_avx512f_enabled = 0;
    uint32_t reg[4];

    __asm__ __volatile__ ("cpuid" :
                          "=a" (reg[0]), "=b" (reg[1]), "=c" (reg[2]), "=d" (reg[3]) :
                          "a" (7), "c" (0));
    is_avx512f_enabled = (( reg[1] & (1 << 16) ) >> 16) &
                         (( reg[1] & (1 << 30) ) >> 30) &
                         (( reg[1] & (1 << 31) ) >> 31);

    return (is_avx512f_enabled) ? 1 : 0;
#else
    return 0;
#endif
}

#ifdef CCL_BFP16_COMPILER

/* float32 -> bfloat16 */
#ifdef CCL_BFP16_TARGET_ATTRIBUTES
void convert_fp32_to_bfp16(const void* src, void* dst) __attribute__((target("avx512bw")));
#endif
void convert_fp32_to_bfp16(const void* src, void* dst)
{
    __m512i y = _mm512_bsrli_epi128(_mm512_loadu_si512(src), 2);
    _mm256_storeu_si256((__m256i*)(dst), _mm512_cvtepi32_epi16(y));
}

/* bfloat16 -> float32 */
#ifdef CCL_BFP16_TARGET_ATTRIBUTES
void convert_bfp16_to_fp32(const void* src, void* dst) __attribute__((target("avx512bw")));
#endif
void convert_bfp16_to_fp32(const void* src, void* dst)
{
    __m512i y = _mm512_cvtepu16_epi32(_mm256_loadu_si256((__m256i const*)src));
    _mm512_storeu_si512(dst, _mm512_bslli_epi128(y, 2));
}

void convert_fp32_to_bfp16_arrays(void* send_buf, void* send_buf_bfp16, int count)
{
    int int_val = 0, int_val_shifted = 0;
    float* send_buf_float = (float*)send_buf;
    int limit = (count / FLOATS_IN_M512) * FLOATS_IN_M512;

    for (int i = 0; i < limit; i += FLOATS_IN_M512)
    {
        convert_fp32_to_bfp16(send_buf_float + i, ((unsigned char*)send_buf_bfp16)+(2 * i));
    }

    /* proceed remaining float's in buffer */
    for (int i = limit; i < count; i ++)
    {
        /* iterate over send_buf_bfp16 */
        int* send_bfp_tail = (int*)(((char*)send_buf_bfp16) + (2 * i));
        /* copy float (4 bytes) data as is to int variable, */
        memcpy(&int_val,&send_buf_float[i], 4);
        /* then perform shift and */
        int_val_shifted = int_val >> BFP16_SHIFT;
        /* save pointer to result */
        *send_bfp_tail = int_val_shifted;
    }
}

void convert_bfp16_to_fp32_arrays(void* recv_buf_bfp16, float* recv_buf, int count)
{
    int int_val = 0, int_val_shifted = 0;
    int limit = (count / FLOATS_IN_M512) * FLOATS_IN_M512;

    for (int i = 0; i < limit; i += FLOATS_IN_M512)
    {
        convert_bfp16_to_fp32((char*)recv_buf_bfp16 + (2 * i), recv_buf+i);
    }

    /* proceed remaining bfp16's in buffer */
    for (int i = limit; i < count; i ++)
    {
        /* iterate over recv_buf_bfp16 */
        int* recv_bfp_tail = (int*)((char*)recv_buf_bfp16 + (2 * i));
        /* copy bfp16 data as is to int variable, */
        memcpy(&int_val,recv_bfp_tail,4);
        /* then perform shift and */
        int_val_shifted = int_val << BFP16_SHIFT;
        /* copy result to output */
        memcpy((recv_buf+i), &int_val_shifted, 4);
    }
}
#endif /* CCL_BFP16_COMPILER */