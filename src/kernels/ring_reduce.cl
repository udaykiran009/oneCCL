#include "common_helpers.h"

#pragma OPENCL EXTENSION cl_intel_subgroups : enable
#pragma OPENCL EXTENSION cl_khr_subgroups : enable

#ifdef KERNEL_DEBUG

#define LOG_INPUT_DATA_START(kern_id) printf("Kernel %zu, wait income data \n", kern_id)

#define LOG_INPUT_DATA_END(kern_id) printf("Kernel %zu, received data \n", kern_id)

#define LOG_OUTGOING_DATA_START(kern_id) printf("Kernel %zu, wait signal to send\n", kern_id)

#define LOG_OUTGOING_DATA_END(kern_id) printf("Kernel %zu, received signal to send\n", kern_id)

#define LOG_SEND_PROGRESS(kern_id, thread_id, flag, desired) \
    printf("Kernel %zu.%d, send %d/%d\n", kern_id, thread_id, flag, desired)

#define LOG_BARRIER_PASSED(kern_id, thread_id) \
    printf("kernel %zu.%d barrier passed\n", kern_id, thread_id);

#define LOG_IN_BARRIER(kern_id, thread_id, flag, desired) \
    printf("kernel %zu.%d barrier %d/%d\n", kern_id, thread_id, flag, desired);

#define DEBUG_BLOCK(block) block

#else

#define LOG_INPUT_DATA_START(kern_id)
#define LOG_INPUT_DATA_END(kern_id)
#define LOG_OUTGOING_DATA_START(kern_id)
#define LOG_OUTGOING_DATA_END(kern_id)
#define LOG_BARRIER_PASSED(kern_id, thread_id)

#define LOG_IN_BARRIER(kern_id, thread_id, flag, desired)

#define DEBUG_BLOCK(block)

#endif

#define PUT_READY_TO_RECEIVE(_sync_flag) \
    if (thread_id == 0) { \
        (*_sync_flag)++; \
    }

#define WAIT_INPUT_DATA(_sync_flag, _desired) \
    if (thread_id == 0) { \
        LOG_INPUT_DATA_START(my_rank); \
        while (1) { \
            if (*_sync_flag == _desired) { \
                LOG_INPUT_DATA_END(my_rank); \
                ++_desired; \
                break; \
            } \
        } \
    }

#define WAIT_SIGNAL_TO_SEND(_sync_flag, _desired) \
    if (thread_id == 0) { \
        LOG_OUTGOING_DATA_START(my_rank); \
        while (_desired != *_sync_flag) { \
        }; \
        LOG_OUTGOING_DATA_END(my_rank); \
        ++_desired; \
    }

#define I_SENT(_sync_flag) \
    if (thread_id == 0) { \
        (*_sync_flag)++; \
    }

/**
 * @param left_wrote_to_me_flag  - located in the memory of the current kernel, left rank uses a pointer to it to notify that he has sent some data.
 * @param i_ready_to_receive_flag - located in the memory of the left peer, left rank uses a pointer to it to check if he can send some data to us
 * @param i_send_to_right_flag - located in the memory of the right kernel. Used by current kernel to notify right kernel that he has sent some data
 * @param right_ready_to_recv_flag - located in the memory of the current kernel. Used by right kernel to notify us it's ready to receive
 */

// Name - unique name suffix for the kernel
// T - type parameter(e.g. float, int4, etc)
// VecSize - vector size of the type. E.g. if float4 is used, VecSize is 4. Note: if just float is used,
// the value must be one as it's used for division inside the kernel.
// Op - A operation parameter(e.g. add(x, y))
// OpName - Operator name which goes to the kernel name, e.g. OpName = add, Op = __add_int(actual function)
#define DEFINE_KERNEL(Name, T, VecSize, Op, OpName) \
    __kernel void reduce_execution_##Name##_##OpName( \
        int my_rank, /*0*/ \
        int comm_size, /*1*/ \
        size_t elems_count, /*2*/ \
        const __global T* input_buffer, /*3*/ \
        __global T* output_buffer, /*4*/ \
\
        __global T* tmp_buffer, /*5*/ \
        __global volatile int* left_wrote_to_me_flag, /*6*/ \
        __global volatile int* i_ready_to_receive_flag, /*7*/ \
\
        __global volatile int* local_barrier_flag, /*8*/ \
\
        __global T* right_temp_buffer, /*9*/ \
        __global volatile int* i_send_to_right_flag, /*10*/ \
        __global volatile int* right_ready_to_recv_flag, /*11*/ \
        int root /*12*/ \
    ) { \
        /* The RING based algorithm,                                                                        \
     where the root rank is the end point.                                                              \
     The direction is from Left -> to Right                                                             \
                                                                                                        \
                          0 (in-between rank)                                                           \
                         / \                                                                            \
    (the farthest rank) 3   1 (in-between rank)                                                         \
        from Root        \ /                                                                            \
                          2 (Root.End point)                                                            \
                                                                                                        \
     Root(2) - consumer                                                                                 \
     In-between ranks(0,1) - consumer producer                                                          \
     The farthest rank(3) - producer */ \
\
        elems_count = elems_count / VecSize; \
        size_t work_group_size = get_global_size(0); \
        size_t segment_count = elems_count / work_group_size; \
        int thread_id = get_global_id(0); \
\
        int ready_to_recv_sync_count = 1; \
        int can_send_sync_count = 1; \
\
        DEBUG_BLOCK( \
            printf("kernel %zu.%d work_group_size: %d\n", my_rank, thread_id, work_group_size)); \
\
        if (my_rank == root) { /*consumer r:ROOT*/ \
            PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
            /*                                                                                              \
            TODO consider prefetch v                                                                    \
            oid prefetch(const __global gentype *p, size_t num_gentypes)                                \
        */ \
            WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
            barrier(CLK_LOCAL_MEM_FENCE); \
            for (size_t i = 0; i < segment_count; i++) { \
                DEBUG_BLOCK(printf("kernel %zu.%d, phase 2. -- temp[%zu] = %f, this[%zu] = %f\n", \
                                   my_rank, \
                                   thread_id, \
                                   i + thread_id, \
                                   tmp_buffer[thread_id + i], \
                                   i + thread_id, \
                                   input_buffer[i + thread_id])); \
                output_buffer[work_group_size * i + thread_id] = \
                    Op(input_buffer[work_group_size * i + thread_id], \
                       tmp_buffer[work_group_size * i + thread_id]); \
            } \
            barrier(CLK_GLOBAL_MEM_FENCE); \
        } \
        else if (my_rank == (root + 1) % comm_size) { /* producer (the farthest rank) */ \
            WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
            barrier(CLK_LOCAL_MEM_FENCE); \
\
            for (size_t i = 0; i < segment_count; i++) { \
                right_temp_buffer[work_group_size * i + thread_id] = \
                    input_buffer[work_group_size * i + thread_id]; \
            } \
\
            barrier(CLK_GLOBAL_MEM_FENCE); \
            I_SENT(i_send_to_right_flag); \
        } \
        else { /* consumer producer (in-between ranks) */ \
            PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
            WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
            /*                                                                                              \
            TODO consider prefetch v                                                                    \
            oid prefetch(const __global gentype *p, size_t num_gentypes)                                \
        */ \
            WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
            barrier(CLK_LOCAL_MEM_FENCE); \
\
            for (size_t i = 0; i < segment_count; i++) { \
                right_temp_buffer[work_group_size * i + thread_id] = \
                    Op(tmp_buffer[work_group_size * i + thread_id], \
                       input_buffer[work_group_size * i + thread_id]); \
            } \
\
            barrier(CLK_GLOBAL_MEM_FENCE); \
            I_SENT(i_send_to_right_flag); \
        } \
\
        DEBUG_BLOCK(printf("kernel %zu.%d completed\n", my_rank, thread_id)); \
    }

// Macro to define kernels for a specific operation for all the supported types.
// Note: for op function we use convention __<OpName>_<type>, where type is the actual type(e.g. int4, float)
#define DEFINE_KERNELS_WITH_OP(OpName) \
    DEFINE_KERNEL(int8, char4, 4, __##OpName##_##char4, OpName) \
    DEFINE_KERNEL(uint8, uchar4, 4, __##OpName##_##uchar4, OpName) \
\
    DEFINE_KERNEL(int16, short4, 4, __##OpName##_##short4, OpName) \
    DEFINE_KERNEL(uint16, ushort4, 4, __##OpName##_##ushort4, OpName) \
\
    DEFINE_KERNEL(int32, int4, 4, __##OpName##_##int4, OpName) \
    DEFINE_KERNEL(uint32, uint4, 4, __##OpName##_##uint4, OpName) \
\
    DEFINE_KERNEL(int64, long4, 4, __##OpName##_##long4, OpName) \
    DEFINE_KERNEL(uint64, ulong4, 4, __##OpName##_##ulong4, OpName) \
\
    /* TODO: implement support for missing types*/ \
    DEFINE_KERNEL(float16, float16, 1, __##OpName##_##float16, OpName) \
    DEFINE_KERNEL(float32, float4, 4, __##OpName##_##float4, OpName) \
    DEFINE_KERNEL(float64, double4, 4, __##OpName##_##double4, OpName) \
    DEFINE_KERNEL(bfloat16, ushort, 1, __##OpName##_##bfloat16, OpName)

#define DEFINE_ADD_OP(T) \
    T __add_##T(T lhs, T rhs) { \
        return lhs + rhs; \
    }

#define DEFINE_MULT_OP(T) \
    T __mult_##T(T lhs, T rhs) { \
        return lhs * rhs; \
    }

#define DEFINE_MIN_OP(T) \
    T __min_##T(T lhs, T rhs) { \
        return min(lhs, rhs); \
    }

#define DEFINE_MAX_OP(T) \
    T __max_##T(T lhs, T rhs) { \
        return max(lhs, rhs); \
    }

#define DEFINE_OPS(T) \
    DEFINE_ADD_OP(T) \
    DEFINE_MULT_OP(T) \
    DEFINE_MIN_OP(T) \
    DEFINE_MAX_OP(T)

// Define Op function for each supported type(use vector types for some of them as required by the kernel)
DEFINE_OPS(char4)
DEFINE_OPS(uchar4)

DEFINE_OPS(short4)
DEFINE_OPS(ushort4)

DEFINE_OPS(int4)
DEFINE_OPS(uint4)

DEFINE_OPS(long4)
DEFINE_OPS(ulong4)

DEFINE_OPS(float4)
DEFINE_OPS(double4)
// Uses integer ops for now since bfloat16 is aliased to ushort for now
DEFINE_OPS(bfloat16)
// TODO: Enable when half is supported
DEFINE_OPS(float16)

// Define the actual kernels
DEFINE_KERNELS_WITH_OP(add)
DEFINE_KERNELS_WITH_OP(mult)
DEFINE_KERNELS_WITH_OP(min)
DEFINE_KERNELS_WITH_OP(max)

// numa
// TODO: vecsize
#define DEFINE_KERNEL_NUMA(Name, T, Op, OpName) \
    __kernel void reduce_execution_numa_##Name##_##OpName( \
        int my_rank, \
        int comm_size, /*1*/ \
        size_t elems_count, /*2*/ \
        const __global T* input_buffer, /*3*/ \
        __global T* output_buffer, /*4*/ \
\
        __global T* tmp_buffer, /*5*/ \
        __global volatile int* left_wrote_to_me_flag, /*6*/ \
        __global volatile int* i_ready_to_receive_flag, /*7*/ \
\
        __global volatile int* local_barrier_flag, /*8*/ \
\
        __global T* right_temp_buffer, /*9*/ \
        __global volatile int* i_send_to_right_flag, /*10*/ \
        __global volatile int* right_ready_to_recv_flag, /*11*/ \
        int root /*12*/) { \
        return; \
    }

DEFINE_KERNEL_NUMA(int8, char4, __add_char4, add)
DEFINE_KERNEL_NUMA(uint8, uchar4, __add_uchar4, add)

DEFINE_KERNEL_NUMA(int16, short4, __add_short4, add)
DEFINE_KERNEL_NUMA(uint16, ushort4, __add_ushort4, add)

DEFINE_KERNEL_NUMA(int32, int4, __add_int4, add)
DEFINE_KERNEL_NUMA(uint32, uint4, __add_uint4, add)

DEFINE_KERNEL_NUMA(int64, long4, __add_long4, add)
DEFINE_KERNEL_NUMA(uint64, ulong4, __add_ulong4, add)

// TODO: implement support for missing types
DEFINE_KERNEL_NUMA(float16, float16, __add_float16, add)
DEFINE_KERNEL_NUMA(float32, float4, __add_float4, add)
DEFINE_KERNEL_NUMA(float64, double4, __add_double4, add)

DEFINE_KERNEL_NUMA(bfloat16, ushort, __add_ushort, add)
