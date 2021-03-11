#include "common.h"

/**
 * @param left_wrote_to_me_flag  - located in the memory of the current kernel, left rank uses a pointer to it to notify that he has sent some data.
 * @param i_ready_to_receive_flag - located in the memory of the left peer, left rank uses a pointer to it to check if he can send some data to us
 * @param i_send_to_right_flag - located in the memory of the right kernel. Used by current kernel to notify right kernel that he has sent some data
 * @param right_ready_to_recv_flag - located in the memory of the current kernel. Used by right kernel to notify us it's ready to receive
 */
// Name - unique name suffix for the kernel
// T is the type parameter(e.g. float, int4, etc)
// VecSize is the vector size of the type. E.g. if float4 is used, VecSize is 4. Note: if just float is used,
// the value must be one as it's used for division inside the kernel.
#define DEFINE_KERNEL(Name, T, VecSize) \
    __kernel void allgatherv_execution_##Name(int my_rank, \
                                              int comm_size, \
                                              ulong elem_count, \
\
                                              __global ulong* recv_elem_counts, \
                                              __global ulong* recv_elem_offsets, \
\
                                              const __global T* input_buffer, \
                                              __global T* output_buffer, \
                                              __global T* right_output_buffer, \
\
                                              __global volatile int* left_wrote_to_me_flag, \
                                              __global volatile int* i_ready_to_receive_flag, \
                                              __global volatile int* i_send_to_right_flag, \
                                              __global volatile int* right_ready_to_recv_flag) { \
        /*Known limitation                                                                                  \
        1) MUST: elems_count >= get_global_size(0) * VecSize * comm_size                                \
        2) MUST: elems_count be aligned with 'get_global_size(0) * VecSize'                             \
        3) TBA: double-buffering                                                                        \
        4) TBA: chunking                                                                                \
        5) TBA: multiple WGs; */ \
\
        int work_rank = my_rank; \
        size_t segment_size = recv_elem_counts[work_rank] / VecSize; /*reduce by vectro T */ \
        size_t segment_offset = recv_elem_offsets[work_rank] / VecSize; \
\
        size_t work_group_size = get_global_size(0); \
        int thread_id = get_global_id(0); \
\
        int ready_to_recv_sync_count = 1; \
        int can_send_sync_count = 1; \
\
        DEBUG_BLOCK(/* int sg_id = get_sub_group_id(); */ \
                    printf("kernel %d.%d work_group_size: %zu, segment_size: %zu\n", \
                           my_rank, \
                           thread_id, \
                           work_group_size, \
                           segment_size /*, sg_id*/)); \
\
        /* 1. copy own data to the right rank */ \
        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
        for (size_t i = 0; i < segment_size; i += work_group_size) { \
            right_output_buffer[thread_id + i + segment_offset] = input_buffer[thread_id + i]; \
        } \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
        I_SENT(i_send_to_right_flag); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
        WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
        DEBUG_BLOCK(printf("kernel %d.%d send complete\n", my_rank, thread_id)); \
\
        for (int iter_idx = 0; iter_idx < comm_size - 2; ++iter_idx) { \
            work_rank = get_left_rank(work_rank, comm_size); \
\
            segment_size = recv_elem_counts[work_rank] / VecSize; \
            segment_offset = recv_elem_offsets[work_rank] / VecSize; \
\
            PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
            WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
            for (size_t i = 0; i < segment_size; i += work_group_size) { \
                right_output_buffer[thread_id + i + segment_offset] = \
                    output_buffer[thread_id + i + segment_offset]; \
            } \
            barrier(CLK_GLOBAL_MEM_FENCE); \
\
            I_SENT(i_send_to_right_flag); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
            WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
        } \
\
        work_rank = my_rank; \
        segment_size = recv_elem_counts[work_rank] / VecSize; \
        segment_offset = recv_elem_offsets[work_rank] / VecSize; \
\
        for (size_t i = 0; i < segment_size; i += work_group_size) { \
            output_buffer[thread_id + i + segment_offset] = input_buffer[thread_id + i]; \
        } \
        barrier(CLK_GLOBAL_MEM_FENCE); \
\
        DEBUG_BLOCK(printf("kernel %d.%d completed\n", my_rank, thread_id)); \
    }

DEFINE_KERNEL(int8, char4, 4)
DEFINE_KERNEL(uint8, uchar4, 4)
DEFINE_KERNEL(int16, short4, 4)
DEFINE_KERNEL(uint16, ushort4, 4)
DEFINE_KERNEL(int32, int4, 4)
DEFINE_KERNEL(uint32, uint4, 4)
DEFINE_KERNEL(int64, long4, 4)
DEFINE_KERNEL(uint64, ulong4, 4)
DEFINE_KERNEL(float16, half, 1)
DEFINE_KERNEL(float32, float4, 4)
DEFINE_KERNEL(float64, double4, 4)
DEFINE_KERNEL(bfloat16, ushort, 1)
