#ifndef A2A_HELPERS_H
#define A2A_HELPERS_H

#include "common_helpers.h"

#define DEFINE_A2A_COMM_DATA(NAME, T) \
    typedef struct __attribute__((packed)) a2a_gpu_comm_data_##NAME { \
        __global T* recv_buf; \
        __global int* ready_to_receive_flag; \
        __global int* data_sent_flag; \
    } a2a_gpu_comm_data_##NAME;

DEFINE_A2A_COMM_DATA(int8, int8_t)
DEFINE_A2A_COMM_DATA(uint8, uint8_t)
DEFINE_A2A_COMM_DATA(int16, int16_t)
DEFINE_A2A_COMM_DATA(uint16, uint16_t)
DEFINE_A2A_COMM_DATA(int32, int32_t)
DEFINE_A2A_COMM_DATA(uint32, uint32_t)
DEFINE_A2A_COMM_DATA(int64, int64_t)
DEFINE_A2A_COMM_DATA(uint64, uint64_t)
DEFINE_A2A_COMM_DATA(float16, float16)
DEFINE_A2A_COMM_DATA(float32, float)
DEFINE_A2A_COMM_DATA(float64, double)
DEFINE_A2A_COMM_DATA(bfloat16, uint16_t)
#endif /* A2A_HELPERS_H */
