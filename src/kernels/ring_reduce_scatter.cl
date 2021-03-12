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
    __kernel void reduce_scatter_execution_##Name##_##OpName( \
        int my_rank, \
        int comm_size, \
        ulong elems_count, /* recv_count */ \
        const __global T* input_buffer, \
        __global T* output_buffer, \
\
        __global T* tmp_buffer, \
        __global sync_flag_type* left_wrote_to_me_flag, \
        __global sync_flag_type* i_ready_to_receive_flag, \
\
        __global volatile int* local_barrier_flag, \
\
        __global T* right_output_buffer, \
        __global T* right_temp_buffer, \
\
        __global sync_flag_type* i_send_to_right_flag, \
        __global sync_flag_type* right_ready_to_recv_flag) { \
        /*Known limitation                                                                              \
      1) MUST: elems_count >= get_global_size(0) * 4 (vector size) * comm_size                      \
      2) MUST: elems_count be aligned with 'get_global_size(0) * 4 (vector size)'                   \
      3) TBA: double-buffering                                                                      \
      4) TBA: chunking                                                                              \
      5) TBA: multiple WGs; */ \
\
        elems_count = elems_count / VecSize; /*reduce by vectro float4 */ \
        size_t segment_size = elems_count / comm_size; \
        size_t work_group_size = get_global_size(0); \
        size_t segment_offset = my_rank * segment_size; \
        int thread_id = get_global_id(0); \
\
        int work_rank = my_rank; \
        int ready_to_recv_sync_count = 1; \
        int can_send_sync_count = 1; \
\
        size_t tmp_buffer_offset = 0; \
        size_t right_tmp_buffer_offset = segment_size; \
\
        DEBUG_BLOCK(/*int sg_id = get_sub_group_id(); */ \
                    printf("kernel %d.%d work_group_size: %zu, segment_size: %zu\n", \
                           my_rank, \
                           thread_id, \
                           work_group_size, \
                           segment_size /*, sg_id*/)); \
\
        /*1. copy own part to the right rank  */ \
        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
        for (size_t i = 0; i < segment_size; i += work_group_size) { \
            right_temp_buffer[thread_id + i] = input_buffer[segment_offset + thread_id + i]; \
        } \
        barrier(CLK_GLOBAL_MEM_FENCE); \
\
        I_SENT(i_send_to_right_flag); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
        WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
        DEBUG_BLOCK(printf("kernel %d.%d send complete\n", my_rank, thread_id)); \
        /*2nd phase - reduce scatter                                                                    \
    get data written by left rank to our temp buffer, reduce with our part and send to right rank*/ \
\
        for (size_t iter_idx = 0; iter_idx < comm_size - 1; ++iter_idx) { \
            work_rank = get_left_rank(work_rank, comm_size); \
            segment_offset = work_rank * segment_size; \
\
            /*left rank has written data to our temp buffer, reduce it with corresponding               \
          element from our initial buffer and send to the right rank*/ \
\
            PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
            WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
            __global T* right_buffer = \
                (iter_idx == (comm_size - 2)) ? right_output_buffer : right_temp_buffer; \
            size_t right_buffer_offset = \
                (iter_idx == (comm_size - 2)) ? 0 : right_tmp_buffer_offset; \
\
            for (size_t i = 0; i < segment_size; i += work_group_size) { \
                DEBUG_BLOCK( \
                    printf("kernel %d.%d, phase 2.%zu -- temp[%zu] = %f, this[%zu] = %d\n", \
                           my_rank, \
                           thread_id, \
                           iter_idx, \
                           segment_offset + thread_id + i, \
                           tmp_buffer[thread_id + i], \
                           segment_offset + thread_id + i, \
                           input_buffer[segment_offset + thread_id + i])); \
                right_buffer[thread_id + i + right_buffer_offset] = \
                    Op(tmp_buffer[thread_id + i + tmp_buffer_offset], \
                       input_buffer[segment_offset + thread_id + i]); \
            } \
            barrier(CLK_GLOBAL_MEM_FENCE); \
\
            I_SENT(i_send_to_right_flag); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
            WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
            SWAP_VARIABLES(tmp_buffer_offset, right_tmp_buffer_offset, size_t); \
        } \
\
        DEBUG_BLOCK(printf( \
            "kernel %d.%d, phase 2 completed, work_rank %d\n", my_rank, thread_id, work_rank)); \
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
