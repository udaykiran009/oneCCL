#include "common.h"

#define SET_PROXY_SIZE(size) \
    if (thread_id == 0) { \
        *right_proxy_size_flag = size; \
    }
#define GET_PROXY_SIZE(size) size = *proxy_size_flag;

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
    __kernel void alltoallv_execution_##Name( \
        int my_rank, /*0*/ \
        int comm_size, /*1*/ \
\
        __global size_t* send_elem_counts, /*2*/ \
        __global size_t* send_elem_offsets, /*3*/ \
        __global size_t* recv_elem_counts, /*4*/ \
        __global size_t* recv_elem_offsets, /*5*/ \
\
        const __global T* input_buffer, /*6*/ \
        __global T* output_buffer, /*7*/ \
        __global T* tmp_buffer, /*8*/ \
        __global T* right_temp_buffer, /*9*/ \
\
        __global volatile int* left_wrote_to_me_flag, /*10*/ \
        __global volatile int* i_ready_to_receive_flag, /*11*/ \
        __global volatile int* proxy_size_flag, /*12*/ \
        __global volatile int* i_send_to_right_flag, /*13*/ \
        __global volatile int* right_ready_to_recv_flag, /*14*/ \
        __global volatile int* right_proxy_size_flag /*15*/ \
    ) { \
        size_t work_group_size = get_global_size(0); \
        int thread_id = get_global_id(0); \
        int work_rank = my_rank; \
\
        int ready_to_recv_sync_count = 1; \
        int can_send_sync_count = 1; \
\
        int proxy_size = 0; \
        size_t prev_segment_size = 0; \
        size_t tmp_segment_offset = 0; \
\
        int output_buffer_offset = 0; \
        int counter = 0; \
        int chunk_counter = 0; \
\
        size_t segment_size = send_elem_counts[work_rank] / VecSize; \
        size_t segment_offset = recv_elem_offsets[work_rank] / VecSize; \
        size_t input_buffer_offset = send_elem_offsets[work_rank] / VecSize; \
\
        /*TODO: consider a new solving of calculating segment_offset*/ \
        /*for (int i = 0; i < work_rank; i++) {                                                              \
        segment_offset = send_elem_counts[i] / VecSize;                                                     \
    }*/ \
\
        for (size_t i = 0; i < segment_size; i += work_group_size) { \
            output_buffer[segment_offset + thread_id + i] = \
                input_buffer[input_buffer_offset + thread_id + i]; \
        } \
\
        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
        barrier(CLK_LOCAL_MEM_FENCE); \
\
        for (int idx = 0; idx < comm_size - 1; idx++) { \
            work_rank = get_right_rank(work_rank, comm_size); \
            segment_size = send_elem_counts[work_rank] / VecSize; \
            segment_offset = send_elem_offsets[work_rank] / VecSize; \
            for (size_t i = 0; i < segment_size; i += work_group_size) { \
                right_temp_buffer[tmp_segment_offset + thread_id + i] = \
                    input_buffer[segment_offset + thread_id + i]; \
            } \
            tmp_segment_offset += segment_size; \
            if (tmp_segment_offset >= segment_size * comm_size) \
                tmp_segment_offset = 0; \
            proxy_size += segment_size; \
        } \
\
        barrier(CLK_LOCAL_MEM_FENCE); \
        SET_PROXY_SIZE(proxy_size); \
        I_SENT(i_send_to_right_flag); \
        WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
        barrier(CLK_LOCAL_MEM_FENCE); \
\
        work_rank = my_rank; \
        for (size_t iter = 0; iter < comm_size - 2; iter++) { \
            work_rank = get_left_rank(work_rank, comm_size); \
            segment_size = recv_elem_counts[work_rank] / VecSize; \
            segment_offset = recv_elem_offsets[work_rank] / VecSize; \
            for (size_t i = 0; i < segment_size; i += work_group_size) { \
                output_buffer[segment_offset + thread_id + i] = tmp_buffer[thread_id + i]; \
            } \
            prev_segment_size = segment_size; \
\
            barrier(CLK_LOCAL_MEM_FENCE); \
            GET_PROXY_SIZE(proxy_size); \
            barrier(CLK_LOCAL_MEM_FENCE); \
\
            proxy_size -= prev_segment_size; \
            segment_size = proxy_size; \
\
            for (size_t i = 0; i < segment_size; i += work_group_size) { \
                right_temp_buffer[thread_id + i] = tmp_buffer[prev_segment_size + thread_id + i]; \
            } \
\
            barrier(CLK_LOCAL_MEM_FENCE); \
\
            PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
            WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
            barrier(CLK_LOCAL_MEM_FENCE); \
\
            SET_PROXY_SIZE(proxy_size); \
\
            I_SENT(i_send_to_right_flag); \
            WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
            barrier(CLK_LOCAL_MEM_FENCE); \
        } \
\
        work_rank = get_left_rank(work_rank, comm_size); \
        segment_size = recv_elem_counts[work_rank] / VecSize; \
        segment_offset = recv_elem_offsets[work_rank] / VecSize; \
        for (size_t i = 0; i < segment_size; i += work_group_size) { \
            output_buffer[segment_offset + thread_id + i] = tmp_buffer[thread_id + i]; \
        } \
        barrier(CLK_LOCAL_MEM_FENCE); \
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
