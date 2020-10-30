#include "common_helpers.h"

#pragma OPENCL EXTENSION cl_intel_subgroups : enable
#pragma OPENCL EXTENSION cl_khr_subgroups : enable

//#define KERNEL_DEBUG
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

#else

#define LOG_INPUT_DATA_START(kern_id)
#define LOG_INPUT_DATA_END(kern_id)
#define LOG_OUTGOING_DATA_START(kern_id)
#define LOG_OUTGOING_DATA_END(kern_id)
#define LOG_BARRIER_PASSED(kern_id, thread_id)

#define LOG_IN_BARRIER(kern_id, thread_id, flag, desired)

#endif

#define PUT_READY_TO_RECEIVE(_sync_flag) \
    if (thread_id == 0) { \
        (*_sync_flag)++; \
    }
/*
#define PUT_READY_TO_RECEIVE(_sync_flag)             \
    if(thread_id == 0)                               \
    {                                                \
        atomic_inc(_sync_flag);                      \
    }
*/

#define I_SENT(_sync_flag) \
    if (thread_id == 0) { \
        (*_sync_flag)++; \
    }
/*
#define I_SENT(_sync_flag)                          \
    if(thread_id == 0)                              \
    {                                               \
        atomic_inc(_sync_flag);                     \
    }
*/
//int _old_value = atomic_cmpxchg(_sync_flag, _desired, _desired);

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

/*
#define WAIT_INPUT_DATA(_sync_flag, _desired)                                       \
    if(thread_id == 0)                                                              \
    {                                                                               \
        LOG_INPUT_DATA_START(my_rank);                                              \
        while(1)                                                                    \
        {                                                                           \
            int _old_value = atomic_add(_sync_flag, 0);                             \
            if(_old_value == _desired)                                              \
            {                                                                       \
                LOG_INPUT_DATA_END(my_rank);                                        \
                ++_desired;                                                         \
                break;                                                              \
            }                                                                       \
        }                                                                           \
    }
*/

#define WAIT_SIGNAL_TO_SEND(_sync_flag, _desired) \
    if (thread_id == 0) { \
        LOG_OUTGOING_DATA_START(my_rank); \
        while (_desired != *_sync_flag) { \
        }; \
        LOG_OUTGOING_DATA_END(my_rank); \
        ++_desired; \
    }

/*
#define WAIT_SIGNAL_TO_SEND(_sync_flag, _desired)                                   \
    if(thread_id == 0)                                                              \
    {                                                                               \
        LOG_OUTGOING_DATA_START(my_rank);                                           \
        while(_desired != atomic_add(_sync_flag, 0)) {};                            \
        LOG_OUTGOING_DATA_END(my_rank);                                             \
        ++_desired;                                                                 \
    }
*/

/*
#define KERNEL_BARRIER(_barrier_flag, _desired, _increment)                         \
    do                                                                              \
    {                                                                               \
        int _barrier_value = atomic_add(_barrier_flag, 0);                          \
        atomic_inc(_barrier_flag);                                                  \
        int _old_value = _barrier_value;                                            \
        while(1)                                                                    \
        {                                                                           \
            / *thread that last reached the barrier will reset it                    \
              other threads may expect to receive _desired value while it can be 0  \
              check if received value is less than initially received* /             \
            if(_old_value == _desired || _old_value < _barrier_value)               \
            {                                                                       \
                BARRIER_PASSED(my_rank, thread_id);                                 \
                break;                                                              \
            }                                                                       \
            IN_BARRIER(my_rank, thread_id, _old_value, _desired);                   \
            _old_value = atomic_add(_barrier_flag, 0);                \
        }                                                                           \
    } while (0);
*/

#define SWAP_VARIABLES(var1, var2, type) \
    do { \
        type tmp; \
        tmp = var1; \
        var1 = var2; \
        var2 = tmp; \
    } while (0);

int get_left_rank(int rank, int comm_size) {
    return rank == 0 ? comm_size - 1 : rank - 1;
}

#ifdef KERNEL_DEBUG
#define DEBUG_BLOCK(block) block
#else
#define DEBUG_BLOCK(block)
#endif

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
#define DEFINE_KERNEL(Name, T, VecSize)                                                                 \
__kernel void allgatherv_execution_## Name(int my_rank,                                              \
                                         int comm_size,                                              \
                                         size_t elem_count,                                             \
                                                                                                        \
                                         __global size_t* recv_elem_counts,                             \
                                         __global size_t* recv_elem_offsets,                            \
                                                                                                        \
                                         const __global T* input_buffer,                                \
                                         __global T* output_buffer,                                     \
                                         __global T* right_output_buffer,                               \
                                                                                                        \
                                         __global volatile int* left_wrote_to_me_flag,                  \
                                         __global volatile int* i_ready_to_receive_flag,                \
                                         __global volatile int* i_send_to_right_flag,                   \
                                         __global volatile int* right_ready_to_recv_flag                \
) {                                                                                                     \
    /*Known limitation                                                                                  \
        1) MUST: elems_count >= get_global_size(0) * VecSize * comm_size                                \
        2) MUST: elems_count be aligned with 'get_global_size(0) * VecSize'                             \
        3) TBA: double-buffering                                                                        \
        4) TBA: chunking                                                                                \
        5) TBA: multiple WGs; */                                                                        \
                                                                                                        \
    int work_rank = my_rank;                                                                         \
    size_t segment_size = recv_elem_counts[work_rank] / VecSize; /*reduce by vectro T */                \
    size_t segment_offset = recv_elem_offsets[work_rank] / VecSize;                                     \
                                                                                                        \
    size_t work_group_size = get_global_size(0);                                                        \
    int thread_id = get_global_id(0);                                                                   \
                                                                                                        \
    int ready_to_recv_sync_count = 1;                                                                   \
    int can_send_sync_count = 1;                                                                        \
                                                                                                        \
    DEBUG_BLOCK(/* int sg_id = get_sub_group_id(); */                                                   \
                printf("kernel %zu.%d work_group_size: %d, segment_size: %d\n",                         \
                    my_rank,                                                                            \
                    thread_id,                                                                          \
                    work_group_size,                                                                    \
                    segment_size /*, sg_id*/));                                                         \
                                                                                                        \
    /* 1. copy own data to the right rank */                                                            \
    PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);                                                      \
    WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);                                 \
    barrier(CLK_LOCAL_MEM_FENCE);                                                                       \
                                                                                                        \
    for (size_t i = 0; i < segment_size; i += work_group_size) {                                        \
        right_output_buffer[thread_id + i + segment_offset] = input_buffer[thread_id + i];              \
    }                                                                                                   \
    barrier(CLK_GLOBAL_MEM_FENCE);                                                                      \
                                                                                                        \
    I_SENT(i_send_to_right_flag);                                                                       \
    WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count);                                   \
    barrier(CLK_LOCAL_MEM_FENCE);                                                                       \
                                                                                                        \
    DEBUG_BLOCK(printf("kernel %zu.%d send complete\n", my_rank, thread_id));                           \
                                                                                                        \
    for (int iter_idx = 0; iter_idx < comm_size - 2; ++iter_idx) {                                   \
        work_rank = get_left_rank(work_rank, comm_size);                                                \
                                                                                                        \
        segment_size = recv_elem_counts[work_rank] / VecSize;                                           \
        segment_offset = recv_elem_offsets[work_rank] / VecSize;                                        \
                                                                                                        \
        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);                                                  \
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);                             \
        barrier(CLK_LOCAL_MEM_FENCE);                                                                   \
                                                                                                        \
        for (size_t i = 0; i < segment_size; i += work_group_size) {                                    \
            right_output_buffer[thread_id + i + segment_offset] =                                       \
                output_buffer[thread_id + i + segment_offset];                                          \
        }                                                                                               \
        barrier(CLK_GLOBAL_MEM_FENCE);                                                                  \
                                                                                                        \
        I_SENT(i_send_to_right_flag);                                                                   \
        WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count);                               \
        barrier(CLK_LOCAL_MEM_FENCE);                                                                   \
    }                                                                                                   \
                                                                                                        \
    work_rank = my_rank;                                                                                \
    segment_size = recv_elem_counts[work_rank] / VecSize;                                               \
    segment_offset = recv_elem_offsets[work_rank] / VecSize;                                            \
                                                                                                        \
    for (size_t i = 0; i < segment_size; i += work_group_size) {                                        \
        output_buffer[thread_id + i + segment_offset] = input_buffer[thread_id + i];                    \
    }                                                                                                   \
    barrier(CLK_GLOBAL_MEM_FENCE);                                                                      \
                                                                                                        \
    DEBUG_BLOCK(printf("kernel %zu.%d completed\n", my_rank, thread_id));                               \
                                                                                                        \
}

DEFINE_KERNEL(int8, char4, 4)
DEFINE_KERNEL(uint8, uchar4, 4)

DEFINE_KERNEL(int16, short4, 4)
DEFINE_KERNEL(uint16, ushort4, 4)

DEFINE_KERNEL(int32, int4, 4)
DEFINE_KERNEL(uint32, uint4, 4)

DEFINE_KERNEL(int64, long4, 4)
DEFINE_KERNEL(uint64, ulong4, 4)

// TODO: implement support for missing types
DEFINE_KERNEL(float16, float16, 1)
DEFINE_KERNEL(float32, float4, 4)
DEFINE_KERNEL(float64, double4, 4)

DEFINE_KERNEL(bfloat16, ushort, 1)

// numa
#define DEFINE_KERNEL_NUMA(Name, T, VecSize)                                                                 \
__kernel void allgatherv_execution_numa_## Name(int my_rank,                                              \
                                         int comm_size,                                              \
                                         size_t elem_count,                                             \
                                                                                                        \
                                         __global size_t* recv_elem_counts,                             \
                                         __global size_t* recv_elem_offsets,                            \
                                                                                                        \
                                         const __global T* input_buffer,                                \
                                         __global T* output_buffer,                                     \
                                         __global T* right_output_buffer,                               \
                                                                                                        \
                                         __global volatile int* left_wrote_to_me_flag,                  \
                                         __global volatile int* i_ready_to_receive_flag,                \
                                         __global volatile int* i_send_to_right_flag,                   \
                                         __global volatile int* right_ready_to_recv_flag                \
) { \
    return; \
}

DEFINE_KERNEL_NUMA(int8, char4, 4)
DEFINE_KERNEL_NUMA(uint8, uchar4, 4)

DEFINE_KERNEL_NUMA(int16, short4, 4)
DEFINE_KERNEL_NUMA(uint16, ushort4, 4)

DEFINE_KERNEL_NUMA(int32, int4, 4)
DEFINE_KERNEL_NUMA(uint32, uint4, 4)

DEFINE_KERNEL_NUMA(int64, long4, 4)
DEFINE_KERNEL_NUMA(uint64, ulong4, 4)

// TODO: implement support for missing types
DEFINE_KERNEL_NUMA(float16, float16, 1)
DEFINE_KERNEL_NUMA(float32, float4, 4)
DEFINE_KERNEL_NUMA(float64, double4, 4)

DEFINE_KERNEL_NUMA(bfloat16, ushort, 1)
