
#include <immintrin.h>
#include <math.h>
#include "base.h"

#define ADDITIONAL_PART 100000000.00000001
#define FLOATS_IN_M512 16
#define BFP16_SHIFT 16
#define bfp16_precision 0.0390625 // 2^-8 = 0.00390625

#define RUN_COLLECTIVE(start_cmd, name)                                    \
  do {                                                                     \
      t = 0;                                                               \
      for (iter_idx = 0; iter_idx < ITERS; iter_idx++)                     \
      {                                                                    \
          t1 = when();                                                     \
          CCL_CALL(start_cmd);                                             \
          CCL_CALL(ccl_wait(request));                                     \
          t2 = when();                                                     \
          t += (t2 - t1);                                                  \
      }                                                                    \
      ccl_barrier(NULL, NULL);                                             \
      printf("[%zu] avg %s time: %8.2lf us\n", rank, name, t / ITERS);     \
      fflush(stdout);                                                      \
  } while (0)

#define CHECK_ERROR(send_buf, recv_buf)                                                                             \
  {                                                                                                                 \
    double log_base2 = log(size)/log(2);                                                                            \
    double g = (log_base2 * bfp16_precision)/(1 - (log_base2 * bfp16_precision));                                   \
    for (size_t i = 0; i < COUNT; i++)                                                                              \
    {                                                                                                               \
      double expected = ((size * (size - 1) / 2) + ((float)(i + ADDITIONAL_PART) * size));                          \
      double max_error = g * expected;                                                                              \
      if (fabs(max_error) < fabs(expected - recv_buf[i]))                                                           \
      {                                                                                                             \
        printf("[%zu] got recvBuf[%zu] = %0.7f, but expected = %0.7f, max_error = %0.16f\n",                        \
                rank, i, recv_buf[i], (float)expected, (double) max_error);                                         \
      ASSERT(0, "unexpected value");                                                                                \
      }                                                                                                             \
    }                                                                                                               \
  }

int ccl_detect_avx512f()
{
  uint32_t reg[4];
  /* Baseline AVX512 capabilities used for oneCCL/bf16 implementation*/
  /* CPUID.(EAX=07H, ECX=0):EBX.AVX512F  [bit 16] */
  /* CPUID.(EAX=07H, ECX=0):EBX.AVX512BW [bit 30] */
  /* CPUID.(EAX=07H, ECX=0):EBX.AVX512VL [bit 31] */

  __asm__ __volatile__ ("cpuid" : "=a" (reg[0]), "=b" (reg[1]), "=c" (reg[2]), "=d" (reg[3])  : "a" (7), "c" (0));
  return (( reg[1] & (1 << 16) ) >> 16) & (( reg[1] & (1 << 30) ) >> 30) & (( reg[1] & (1 << 31) ) >> 31);
}

// float32 -> bfloat16
#if defined(__clang__) || defined(__GNUC__) && !defined(__ICC)
void convert_fp32_to_bf16(const void* src, void* dst) __attribute__((target("avx512bw")));
#endif
void convert_fp32_to_bf16(const void* src, void* dst) {
    __m512i y = _mm512_bsrli_epi128(_mm512_loadu_si512(src), 2);
    _mm256_storeu_si256((__m256i*)(dst), _mm512_cvtepi32_epi16(y));
}

// bfloat16 -> float32
#if defined(__clang__) || defined(__GNUC__) && !defined(__ICC)
void convert_bf16_to_fp32(const void* src, void* dst) __attribute__((target("avx512bw")));
#endif
void convert_bf16_to_fp32(const void* src, void* dst) {
    __m512i y = _mm512_cvtepu16_epi32(_mm256_loadu_si256((__m256i const*)src));
    _mm512_storeu_si512(dst, _mm512_bslli_epi128(y, 2));
}

void convert_fp32_to_bf16_arrays(float* send_buf, void* send_buf_bfp16)
{
    int int_val = 0, int_val_shifted = 0;
    for (int i = 0; i < (COUNT / FLOATS_IN_M512) * FLOATS_IN_M512; i += FLOATS_IN_M512)
    {
        convert_fp32_to_bf16(send_buf + i, ((char*)send_buf_bfp16)+(2 * i));
    }
    //proceed remaining float's in buffer
    for (int i = (COUNT / FLOATS_IN_M512) * FLOATS_IN_M512; i < COUNT; i ++)
    {
        //iterate over send_buf_bfp16
        int* send_bfp_tail = (int*)(((char*)send_buf_bfp16) + (2 * i));
        //copy float (4 bytes) data as is to int variable, 
        memcpy(&int_val,&send_buf[i], 4);
        //then perform shift and
        int_val_shifted = int_val >> BFP16_SHIFT;
        //save pointer to result
        *send_bfp_tail = int_val_shifted;
    }
}

void convert_bf16_to_fp32_arrays(void* recv_buf_bfp16, float* recv_buf)
{
    int int_val = 0, int_val_shifted = 0;
    for (int i = 0; i < (COUNT / FLOATS_IN_M512) * FLOATS_IN_M512; i += FLOATS_IN_M512)
    {
        convert_bf16_to_fp32((char*)recv_buf_bfp16 + (2 * i), recv_buf+i);
    }
    //proceed remaining bfp16's in buffer
    for (int i = (COUNT / FLOATS_IN_M512) * FLOATS_IN_M512; i < COUNT; i ++)
    {
        //iterate over recv_buf_bfp16
        int* recv_bfp_tail = (int*)((char*)recv_buf_bfp16 + (2 * i));
        //copy bfp16 data as is to int variable, 
        memcpy(&int_val,recv_bfp_tail,4);
        //then perform shift and
        int_val_shifted = int_val << BFP16_SHIFT;
        //copy result to output
        memcpy((recv_buf+i), &int_val_shifted, 4);
    }
}
int main()
{
    float* send_buf = (float*)malloc(sizeof(float) * COUNT);
    float* recv_buf = (float*)malloc(sizeof(float) * COUNT);
    //bfp16 = 2 bytes
    void* recv_buf_bfp16 = (short *)malloc(sizeof(2) * COUNT);
    void* send_buf_bfp16 = (short *)malloc(sizeof(2) * COUNT);
    test_init();
    for (idx = 0; idx < COUNT; idx++)
    {
        send_buf[idx] = rank + idx + ADDITIONAL_PART;
        recv_buf[idx] = 0.0;
    }
    if (ccl_detect_avx512f() == 0)
    {
        printf("WARNING: avx512f unavailable, bfp16 allreduce has been fall back to float allreduce\n");
        RUN_COLLECTIVE(ccl_allreduce(send_buf, recv_buf, COUNT, ccl_dtype_float, ccl_reduction_sum, &coll_attr, NULL, NULL, &request),
                      "float allreduce");
    }
    else
    {
        convert_fp32_to_bf16_arrays(send_buf, send_buf_bfp16);
        RUN_COLLECTIVE(ccl_allreduce(send_buf_bfp16, recv_buf_bfp16, COUNT, ccl_dtype_bfp16, ccl_reduction_sum, &coll_attr, NULL, NULL, &request),
                      "bfp16 allreduce");
        convert_bf16_to_fp32_arrays(recv_buf_bfp16, recv_buf);
    }
    CHECK_ERROR(send_buf, recv_buf);
    test_finalize();
    free(send_buf);
    free(recv_buf);
    free(send_buf_bfp16);
    free(recv_buf_bfp16);
    if (rank == 0)
      printf("PASSED\n");
    return 0;
}
