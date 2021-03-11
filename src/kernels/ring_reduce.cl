#include "common.h"

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
        ulong elems_count, /*2*/ \
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
            printf("kernel %d.%d work_group_size: %zu\n", my_rank, thread_id, work_group_size)); \
\
        if (my_rank == root) { /*consumer r:ROOT*/ \
            PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
            /*                                                                                              \
            TODO consider prefetch v                                                                    \
            oid prefetch(const __global gentype *p, size_t num_gentypes)                                \
        */ \
            WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
            for (size_t i = 0; i < segment_count; i++) { \
                DEBUG_BLOCK(printf("kernel %d.%d, phase 2. -- temp[%zu] = %f, this[%zu] = %f\n", \
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
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
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
            work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
            WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
            /*                                                                                              \
            TODO consider prefetch v                                                                    \
            oid prefetch(const __global gentype *p, size_t num_gentypes)                                \
        */ \
            WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
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
        DEBUG_BLOCK(printf("kernel %d.%d completed\n", my_rank, thread_id)); \
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
    DEFINE_KERNEL(float32, float4, 4, __##OpName##_##float4, OpName) \
    DEFINE_KERNEL(float64, double4, 4, __##OpName##_##double4, OpName)

#define DEFINE_KERNELS_WITH_BF16OP(OpName) \
    DEFINE_KERNEL(bfloat16, ushort, 1, __##OpName##_##ushort, OpName)

#define DEFINE_KERNELS_WITH_LP_OP(OpName) \
    DEFINE_KERNEL(bfloat16, ushort, 1, __##OpName##_##ushort, OpName) \
    DEFINE_KERNEL(float16, half, 1, __##OpName##_##half, OpName)

#define DEFINE_OPS(T) \
    DEFINE_SUM_OP(T) \
    DEFINE_PROD_OP(T) \
    DEFINE_MIN_OP(T) \
    DEFINE_MAX_OP(T)

#define DEFINE_BF16OPS(T) \
    DEFINE_BF16SUM_OP(T) \
    DEFINE_BF16PROD_OP(T) \
    DEFINE_BF16MIN_OP(T) \
    DEFINE_BF16MAX_OP(T)

#define DEFINE_FP16OPS(T) \
    DEFINE_FP16SUM_OP(T) \
    DEFINE_FP16PROD_OP(T) \
    DEFINE_FP16MIN_OP(T) \
    DEFINE_FP16MAX_OP(T)

// Define Op function for each supported type(use vector types for some of them as required by the kernel)
DEFINE_OPS(char4)
DEFINE_OPS(uchar4)
DEFINE_OPS(short4)
DEFINE_OPS(ushort4)
DEFINE_OPS(int4)
DEFINE_OPS(uint4)
DEFINE_OPS(long4)
DEFINE_OPS(ulong4)
DEFINE_FP16OPS(half)
DEFINE_OPS(float4)
DEFINE_OPS(double4)
DEFINE_BF16OPS(ushort)

// Define the actual kernels
DEFINE_KERNELS_WITH_OP(sum)
DEFINE_KERNELS_WITH_OP(prod)
DEFINE_KERNELS_WITH_OP(min)
DEFINE_KERNELS_WITH_OP(max)

DEFINE_KERNELS_WITH_LP_OP(sum)
DEFINE_KERNELS_WITH_LP_OP(prod)
DEFINE_KERNELS_WITH_LP_OP(min)
DEFINE_KERNELS_WITH_LP_OP(max)
