#ifndef A2A_HELPERS_H
#define A2A_HELPERS_H

#include "common_helpers.h"

#define DEFINE_A2A_COMM_DATA(T) \
    typedef struct __attribute__((packed)) a2a_gpu_comm_data_##T {      \
        __global T* recv_buf;                                           \
        __global int* ready_to_receive_flag;                            \
        __global int* data_sent_flag;                                   \
    } a2a_gpu_comm_data_##T;

#ifdef HOST_CTX
using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using bf16 = ushort;
#endif

// TODO: check whether we need to have aliases here, e.g. int8_t
DEFINE_A2A_COMM_DATA(char)
DEFINE_A2A_COMM_DATA(uchar)

DEFINE_A2A_COMM_DATA(short)
DEFINE_A2A_COMM_DATA(ushort)

DEFINE_A2A_COMM_DATA(int)
DEFINE_A2A_COMM_DATA(uint)

DEFINE_A2A_COMM_DATA(long)
DEFINE_A2A_COMM_DATA(ulong)

DEFINE_A2A_COMM_DATA(float)
DEFINE_A2A_COMM_DATA(double)

DEFINE_A2A_COMM_DATA(bf16)
#endif /* A2A_HELPERS_H */
