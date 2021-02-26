#include "common_helpers.h"

#pragma OPENCL EXTENSION cl_intel_subgroups : enable
#pragma OPENCL EXTENSION cl_khr_subgroups : enable

//#define KERNEL_DEBUG
#ifdef KERNEL_DEBUG

#define LOG_INPUT_DATA_START(kern_id) printf("Kernel %zu, wait income data \n", kern_id)

#define LOG_INPUT_DATA_END(kern_id) printf("Kernel %zu, received data \n", kern_id)

#define LOG_OUTGOING_DATA_START(kern_id) printf("Kernel %zu, wait signal to send\n", kern_id)

#define LOG_OUTGOING_DATA_END(kern_id) printf("Kernel %zu, received signal to send\n", kern_id)

#define LOG_SEND_PROGRESS(kern_id, work_item_id, flag, desired) \
    printf("Kernel %zu.%d, send %d/%d\n", kern_id, work_item_id, flag, desired)

#define LOG_BARRIER_PASSED(kern_id, work_item_id) \
    printf("kernel %zu.%d barrier passed\n", kern_id, work_item_id);

#define LOG_IN_BARRIER(kern_id, work_item_id, flag, desired) \
    printf("kernel %zu.%d barrier %d/%d\n", kern_id, work_item_id, flag, desired);

#else

#define LOG_INPUT_DATA_START(kern_id)
#define LOG_INPUT_DATA_END(kern_id)
#define LOG_OUTGOING_DATA_START(kern_id)
#define LOG_OUTGOING_DATA_END(kern_id)
#define LOG_BARRIER_PASSED(kern_id, work_item_id)

#define LOG_IN_BARRIER(kern_id, work_item_id, flag, desired)

#endif

#define PUT_READY_TO_RECEIVE(_sync_flag) \
    if (work_item_id == 0) { \
        (*_sync_flag)++; \
    }

#define I_SENT(_sync_flag) \
    if (work_item_id == 0) { \
        (*_sync_flag)++; \
    }

#define WAIT_INPUT_DATA(_sync_flag, _desired) \
    if (work_item_id == 0) { \
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
    if (work_item_id == 0) { \
        LOG_OUTGOING_DATA_START(my_rank); \
        while (_desired != *_sync_flag) { \
        }; \
        LOG_OUTGOING_DATA_END(my_rank); \
        ++_desired; \
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
#define DEFINE_KERNEL(Name, T, VecSize) \
    __kernel void bcast_execution_##Name( \
        int my_rank, \
        int comm_size, /* 1 */ \
        ulong elems_count, /* 2 */ \
\
        /* const __global T* input_buffer,*/ /* 3 */ /* __global T* output_buffer,*/ /* 4 */ \
        __global T* buffer, /* 3 */ \
\
        __global volatile int* left_wrote_to_me_flag, /* 5 */ \
        __global volatile int* i_ready_to_receive_flag, /* 6 */ \
\
        __global volatile int* local_barrier_flag, /* 7 */ \
\
        __global T* right_buffer, /* 8 */ \
        __global volatile int* i_send_to_right_flag, /* 9 */ \
        __global volatile int* right_ready_to_recv_flag, /* 10 */ \
        int root /* 11 */ \
    ) { \
        elems_count = elems_count / VecSize; /*bcast by vector T*/ \
        size_t work_group_size = get_global_size(0); \
        size_t last_rank = root ? root - 1 : comm_size - 1; \
        size_t segment_count = elems_count / work_group_size; \
        int work_item_id = get_global_id(0); \
\
        int ready_to_recv_sync_count = 1; \
        int can_send_sync_count = 1; \
\
        DEBUG_BLOCK( \
            printf("kernel %zu.%d work_group_size: %d, elems_count: %zu, segment_count %zu\n", \
                   my_rank, \
                   work_item_id, \
                   work_group_size, \
                   elems_count, \
                   segment_count)); \
\
        if (my_rank == root) { \
            WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
            for (size_t i = 0; i < segment_count; i++) { \
                right_buffer[work_group_size * i + work_item_id] = \
                    buffer[work_group_size * i + work_item_id]; \
                /* input_buffer[work_group_size * i + work_item_id]; */ \
            } \
            barrier(CLK_GLOBAL_MEM_FENCE); \
\
            I_SENT(i_send_to_right_flag); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
            /* for (size_t i = 0; i < segment_count; i++) {                                                         \
              output_buffer[work_group_size * i + work_item_id] =                                               \
                 input_buffer[work_group_size * i + work_item_id];                                              \
         }                                                                                                      \
         barrier(CLK_GLOBAL_MEM_FENCE); */ \
        } \
        else { \
            PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
            WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
            if (my_rank != last_rank) { \
                WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
                work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
                for (size_t i = 0; i < segment_count; i++) { \
                    right_buffer[work_group_size * i + work_item_id] = \
                        buffer[work_group_size * i + work_item_id]; \
                    /* output_buffer[work_group_size * i + work_item_id]; */ \
                } \
                barrier(CLK_GLOBAL_MEM_FENCE); \
\
                I_SENT(i_send_to_right_flag); \
                work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
            } \
        } \
\
        DEBUG_BLOCK(printf("kernel %zu.%d completed\n", my_rank, work_item_id)); \
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
#define DEFINE_KERNEL_NUMA(Name, T, VecSize) \
    __kernel void bcast_execution_numa_##Name( \
        int my_rank, \
        int comm_size, /* 1 */ \
        ulong elems_count, /* 2 */ \
\
        /* const __global T* input_buffer,*/ /* 3 */ /* __global T* output_buffer,*/ /* 4 */ \
        __global T* buffer, /* 3 */ \
\
        __global volatile int* left_wrote_to_me_flag, /* 5 */ \
        __global volatile int* i_ready_to_receive_flag, /* 6 */ \
\
        __global volatile int* local_barrier_flag, /* 7 */ \
\
        __global T* right_buffer, /* 8 */ \
        __global volatile int* i_send_to_right_flag, /* 9 */ \
        __global volatile int* right_ready_to_recv_flag, /* 10 */ \
        int root /* 11 */ \
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
