#include "../common.h"
#include "../shared.h"

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
    __kernel void bcast_execution_##Name(int my_rank, \
                                         int comm_size, /* 1 */ \
                                         __global T* buffer, /* 2 */ \
                                         __global T* right_buffer, /* 3 */ \
                                         int root /* 4 */ \
    ) { \
        size_t idx = get_global_id(0); \
        right_buffer[idx] = buffer[idx]; \
    }

#define DEFINE_KERNELL_MONOLITHIC(Name, T, VecSize) \
    __kernel void bcast_execution_monolithic##Name( \
        int my_rank, \
        int comm_size, /* 1 */ \
        ulong elems_count, /* 2 */ \
\
        /* const __global T* input_buffer,*/ /* 3 */ /* __global T* output_buffer,*/ /* 4 */ \
        __global T* buffer, /* 3 */ \
\
        __global sync_flag_type* left_wrote_to_me_flag, /* 5 */ \
        __global sync_flag_type* i_ready_to_receive_flag, /* 6 */ \
\
        __global volatile int* local_barrier_flag, /* 7 */ \
\
        __global T* right_buffer, /* 8 */ \
        __global sync_flag_type* i_send_to_right_flag, /* 9 */ \
        __global sync_flag_type* right_ready_to_recv_flag, /* 10 */ \
        int root /* 11 */ \
    ) { \
        elems_count = elems_count / VecSize; /*bcast by vector T*/ \
        size_t work_group_size = get_global_size(0); \
        size_t last_rank = root ? root - 1 : comm_size - 1; \
        size_t segment_count = elems_count / work_group_size; \
        int thread_id = get_global_id(0); \
\
        int ready_to_recv_sync_count = 1; \
        int can_send_sync_count = 1; \
\
        DEBUG_BLOCK( \
            printf("kernel %d.%d work_group_size: %zu, elems_count: %zu, segment_count %zu\n", \
                   my_rank, \
                   thread_id, \
                   work_group_size, \
                   elems_count, \
                   segment_count)); \
\
        if (my_rank == root) { \
            WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
            for (size_t i = 0; i + thread_id < elems_count; i += work_group_size) { \
                right_buffer[i + thread_id] = buffer[i + thread_id]; \
                /* input_buffer[work_group_size * i + thread_id]; */ \
            } \
            barrier(CLK_GLOBAL_MEM_FENCE); \
\
            I_SENT(i_send_to_right_flag); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
            /* for (size_t i = 0; i < segment_count; i++) {                                                         \
              output_buffer[work_group_size * i + thread_id] =                                               \
                 input_buffer[work_group_size * i + thread_id];                                              \
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
                for (size_t i = 0; i + thread_id < elems_count; i += work_group_size) { \
                    right_buffer[i + thread_id] = buffer[i + thread_id]; \
                    /* output_buffer[work_group_size * i + thread_id]; */ \
                } \
                barrier(CLK_GLOBAL_MEM_FENCE); \
\
                I_SENT(i_send_to_right_flag); \
                work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
            } \
        } \
\
        DEBUG_BLOCK(printf("kernel %d.%d completed\n", my_rank, thread_id)); \
    }

#define VEC_SIZE RING_BCAST_VEC_SIZE

DEFINE_KERNEL(int8, char, VEC_SIZE)
DEFINE_KERNEL(uint8, uchar, VEC_SIZE)
DEFINE_KERNEL(int16, short, VEC_SIZE)
DEFINE_KERNEL(uint16, ushort, VEC_SIZE)
DEFINE_KERNEL(int32, int, VEC_SIZE)
DEFINE_KERNEL(uint32, uint, VEC_SIZE)
DEFINE_KERNEL(int64, long, VEC_SIZE)
DEFINE_KERNEL(uint64, ulong, VEC_SIZE)
DEFINE_KERNEL(float16, half, VEC_SIZE)
DEFINE_KERNEL(float32, float, VEC_SIZE)
DEFINE_KERNEL(float64, double, VEC_SIZE)
DEFINE_KERNEL(bfloat16, ushort, VEC_SIZE)
