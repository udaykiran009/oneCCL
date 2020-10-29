#include "common_helpers.h"
#include "lp.h"

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

size_t get_left_rank(size_t rank, size_t comm_size) {
    return rank == 0 ? comm_size - 1 : rank - 1;
}

/**
 * @param left_wrote_to_me_flag  - located in the memory of the current kernel, left rank uses a pointer to it to notify that he has sent some data.
 * @param i_ready_to_receive_flag - located in the memory of the left peer, left rank uses a pointer to it to check if he can send some data to us
 * @param i_send_to_right_flag - located in the memory of the right kernel. Used by current kernel to notify right kernel that he has sent some data
 * @param right_ready_to_recv_flag - located in the memory of the current kernel. Used by right kernel to notify us it's ready to receive
 */

#ifdef KERNEL_DEBUG
#define DEBUG_BLOCK(block) block
#else
#define DEBUG_BLOCK(block)
#endif

// Name - unique name suffix for the kernel
// T - type parameter(e.g. float, int4, etc)
// VecSize - vector size of the type. E.g. if float4 is used, VecSize is 4. Note: if just float is used,
// the value must be one as it's used for division inside the kernel.
// Op - A operation parameter(e.g. add(x, y))
// OpName - Operator name which goes to the kernel name, e.g. OpName = add, Op = __add_int(actual function)
#define DEFINE_KERNEL(Name, T, VecSize, Op, OpName)                                                         \
__kernel void allreduce_execution_##Name##_##OpName(size_t my_rank,                                         \
                                        size_t comm_size,                                                   \
                                        size_t elems_count,                                                 \
                                        const __global T* input_buffer,                                     \
                                        __global T* output_buffer,                                          \
                                                                                                            \
                                        __global T* tmp_buffer,                                             \
                                        __global volatile int* left_wrote_to_me_flag,                       \
                                        __global volatile int* i_ready_to_receive_flag,                     \
                                                                                                            \
                                        __global volatile int* local_barrier_flag,                          \
                                                                                                            \
                                        __global T* right_temp_buffer,                                      \
                                        __global volatile int* i_send_to_right_flag,                        \
                                        __global volatile int* right_ready_to_recv_flag) {                  \
    /*Known limitation                                                                                      \
      1) MUST: elems_count >= get_global_size(0) * 4 (vector size) * comm_size                              \
      2) MUST: elems_count be aligned with 'get_global_size(0) * 4 (vector size)'                           \
      3) TBA: double-buffering                                                                              \
      4) TBA: chunking                                                                                      \
      5) TBA: multiple WGs; */                                                                              \
                                                                                                            \
    elems_count = elems_count / VecSize; /*reduce by vectro T */                                            \
    size_t segment_size = elems_count / comm_size;                                                          \
    size_t work_group_size = get_global_size(0);                                                            \
    size_t my_rank_buffer_start_idx = my_rank * segment_size;                                               \
    int thread_id = get_global_id(0);                                                                       \
                                                                                                            \
    size_t work_rank = my_rank;                                                                             \
    int ready_to_recv_sync_count = 1;                                                                       \
    int can_send_sync_count = 1;                                                                            \
                                                                                                            \
    size_t tmp_buffer_offset = 0;                                                                           \
    size_t right_tmp_buffer_offset = segment_size;                                                          \
                                                                                                            \
    DEBUG_BLOCK(/*int sg_id = get_sub_group_id();*/                                                         \
                printf("kernel %zu.%d work_group_size: %d, segment_size: %d\n",                             \
                    my_rank,                                                                                \
                    thread_id,                                                                              \
                    work_group_size,                                                                        \
                    segment_size /*, sg_id*/));                                                             \
                                                                                                            \
    /*1. copy own part to the right rank*/                                                                  \
    PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);                                                          \
                                                                                                            \
    /*aka send*/                                                                                            \
    WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);                                     \
    barrier(CLK_LOCAL_MEM_FENCE);                                                                           \
                                                                                                            \
    for (size_t i = 0; i < segment_size; i += work_group_size) {                                            \
        right_temp_buffer[thread_id + i] = input_buffer[my_rank_buffer_start_idx + thread_id + i];          \
    }                                                                                                       \
    barrier(CLK_GLOBAL_MEM_FENCE);                                                                          \
                                                                                                            \
    I_SENT(i_send_to_right_flag);                                                                           \
                                                                                                            \
    /*aka receive*/                                                                                         \
    WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count);                                       \
    barrier(CLK_LOCAL_MEM_FENCE);                                                                           \
                                                                                                            \
    DEBUG_BLOCK(printf("kernel %zu.%d send complete\n", my_rank, thread_id));                               \
    /*2nd phase - reduce scatter                                                                            \
      get data written by left rank to our temp buffer, reduce with our part and send to right rank*/       \
                                                                                                            \
    for (size_t iter_idx = 0; iter_idx < comm_size - 2; ++iter_idx) {                                       \
        work_rank = get_left_rank(work_rank, comm_size);                                                    \
        size_t segment_offset = work_rank * segment_size;                                                   \
                                                                                                            \
        /*left rank has written data to our temp buffer, reduce it with corresponding element               \
        from our initial buffer                                                                             \
          and send to the right rank  */                                                                    \
                                                                                                            \
        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);                                                      \
        /*aka send*/                                                                                        \
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);                                 \
        barrier(CLK_LOCAL_MEM_FENCE);                                                                       \
                                                                                                            \
        for (size_t i = 0; i < segment_size; i += work_group_size) {                                        \
            DEBUG_BLOCK(printf("kernel %zu.%d, phase 2.%zu -- temp[%zu] = %f, this[%zu] = %f\n",            \
                            my_rank,                                                                        \
                            thread_id,                                                                      \
                            iter_idx,                                                                       \
                            segment_offset + thread_id + i,                                                 \
                            tmp_buffer[thread_id + i],                                                      \
                            segment_offset + thread_id + i,                                                 \
                            input_buffer[segment_offset + thread_id + i]));                                 \
            right_temp_buffer[thread_id + i + right_tmp_buffer_offset] =                                    \
                Op(tmp_buffer[thread_id + i + tmp_buffer_offset],                                           \
                   input_buffer[segment_offset + thread_id + i]);                                           \
        }                                                                                                   \
        barrier(CLK_GLOBAL_MEM_FENCE);                                                                      \
        I_SENT(i_send_to_right_flag);                                                                       \
                                                                                                            \
        /*aka receive*/                                                                                     \
        WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count);                                   \
        barrier(CLK_LOCAL_MEM_FENCE);                                                                       \
                                                                                                            \
        SWAP_VARIABLES(tmp_buffer_offset, right_tmp_buffer_offset, size_t);                                 \
    }                                                                                                       \
                                                                                                            \
    DEBUG_BLOCK(printf("kernel %zu.%d, phase 2 completed, work_rank %zu\n", my_rank, thread_id, work_rank));\
                                                                                                            \
    /* Local reduction */                                                                                   \
    PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);                                                          \
    /*aka send*/                                                                                            \
    WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);                                     \
    barrier(CLK_LOCAL_MEM_FENCE);                                                                           \
                                                                                                            \
    work_rank = get_left_rank(work_rank, comm_size);                                                        \
    size_t segment_offset = work_rank * segment_size;                                                       \
                                                                                                            \
    /*left rank has written data to our temp buffer,                                                        \
      reduce it with corresponding element from our initial buffer                                          \
      and put to output buffer*/                                                                            \
                                                                                                            \
    for (size_t i = 0; i < segment_size; i += work_group_size) {                                            \
        output_buffer[segment_offset + thread_id + i] =                                                     \
            Op(tmp_buffer[thread_id + i + tmp_buffer_offset],                                               \
               input_buffer[segment_offset + thread_id + i]);                                               \
        right_temp_buffer[thread_id + i + right_tmp_buffer_offset] =                                        \
            output_buffer[segment_offset + thread_id + i];                                                  \
        /* We should be able to issue 2 stores for the same data - local and remote,                        \
            no need for local read */                                                                       \
    }                                                                                                       \
    barrier(CLK_GLOBAL_MEM_FENCE);                                                                          \
    I_SENT(i_send_to_right_flag);                                                                           \
                                                                                                            \
    /*aka receive*/                                                                                         \
    WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count);                                       \
    barrier(CLK_LOCAL_MEM_FENCE);                                                                           \
                                                                                                            \
    SWAP_VARIABLES(tmp_buffer_offset, right_tmp_buffer_offset, size_t);                                     \
                                                                                                            \
    DEBUG_BLOCK(printf("kernel %zu.%d, phase 2 completed, work_rank %zu\n", my_rank, thread_id, work_rank));\
                                                                                                            \
    work_rank = my_rank;                                                                                    \
    /*3rd phase - allgather                                                                                 \
      copy ready data from temp buffer by [segment_offset] offset, reduce                                   \
      and send data by [work_rank] offset */                                                                \
    for (size_t iter_idx = 0; iter_idx < comm_size - 2; ++iter_idx) {                                       \
        segment_offset = work_rank * segment_size;                                                          \
        /*copy reduced value to initial buffer*/                                                            \
                                                                                                            \
        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);                                                      \
                                                                                                            \
        /*aka send*/                                                                                        \
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);                                 \
        barrier(CLK_LOCAL_MEM_FENCE);                                                                       \
        for (size_t i = 0; i < segment_size; i += work_group_size) {                                        \
            output_buffer[segment_offset + thread_id + i] =                                                 \
                tmp_buffer[thread_id + i + tmp_buffer_offset];                                              \
                                                                                                            \
            DEBUG_BLOCK(printf("kernel %zu.%d, phase 3.%zu -- send %f to idx %zu, rank %zu\n",              \
                            my_rank,                                                                        \
                            thread_id,                                                                      \
                            iter_idx,                                                                       \
                            tmp_buffer[thread_id + i],                                                      \
                            segment_offset + thread_id,                                                     \
                            work_rank + i));                                                                \
            /*TODO optimize for local memory to avoid double read for global?*/                             \
            right_temp_buffer[thread_id + i + right_tmp_buffer_offset] =                                    \
                tmp_buffer[thread_id + i + tmp_buffer_offset];                                              \
        }                                                                                                   \
        barrier(CLK_GLOBAL_MEM_FENCE);                                                                      \
        I_SENT(i_send_to_right_flag);                                                                       \
        /*aka receive*/                                                                                     \
        WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count);                                   \
                                                                                                            \
        barrier(CLK_LOCAL_MEM_FENCE);                                                                       \
        work_rank = get_left_rank(work_rank, comm_size);                                                    \
                                                                                                            \
        SWAP_VARIABLES(tmp_buffer_offset, right_tmp_buffer_offset, size_t);                                 \
    }                                                                                                       \
                                                                                                            \
    /* Copy from tmp to output buf*/                                                                        \
    PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);                                                          \
                                                                                                            \
    /*aka send*/                                                                                            \
    WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);                                     \
    barrier(CLK_LOCAL_MEM_FENCE);                                                                           \
    segment_offset = work_rank * segment_size;                                                              \
    for (size_t i = 0; i < segment_size; i += work_group_size) {                                            \
        output_buffer[segment_offset + thread_id + i] =                                                     \
            tmp_buffer[thread_id + i + tmp_buffer_offset];                                                  \
    }                                                                                                       \
                                                                                                            \
    DEBUG_BLOCK(printf("kernel %zu.%d completed\n", my_rank, thread_id));                                   \
}

// Macro to define kernels for a specific operation for all the supported types.
// Note: for op function we use convention __<OpName>_<type>, where type is the actual type(e.g. int4, float)
#define DEFINE_KERNELS_WITH_OP(OpName)                                      \
    DEFINE_KERNEL(int8, char4, 4, __##OpName##_##char4, OpName)           \
    DEFINE_KERNEL(uint8, uchar4, 4, __##OpName##_##uchar4, OpName)        \
                                                                            \
    DEFINE_KERNEL(int16, short4, 4, __##OpName##_##short4, OpName)        \
    DEFINE_KERNEL(uint16, ushort4, 4, __##OpName##_##ushort4, OpName)     \
                                                                            \
    DEFINE_KERNEL(int32, int4, 4, __##OpName##_##int4, OpName)            \
    DEFINE_KERNEL(uint32, uint4, 4, __##OpName##_##uint4, OpName)         \
                                                                            \
    DEFINE_KERNEL(int64, long4, 4, __##OpName##_##long4, OpName)          \
    DEFINE_KERNEL(uint64, ulong4, 4, __##OpName##_##ulong4, OpName)       \
                                                                            \
    DEFINE_KERNEL(float32, float4, 4, __##OpName##_##float4, OpName)      \
    DEFINE_KERNEL(float64, double4, 4, __##OpName##_##double4, OpName)    \
    /* TODO: implement support for missing types*/                          \
    /*DEFINE_KERNEL(float16, half, 1, __##OpName##_##half, OpName)*/

#define DEFINE_KERNELS_WITH_BF16OP(OpName)                                  \
    DEFINE_KERNEL(bfloat16, ushort, 1, __##OpName##_##ushort, OpName)

#define DEFINE_ADD_OP(T)                                    \
    T __add_##T(T lhs, T rhs) {                             \
        return lhs + rhs;                                   \
    }

#define DEFINE_MULT_OP(T)                                   \
    T __mult_##T(T lhs, T rhs) {                            \
       return lhs * rhs;                                    \
    }

#define DEFINE_MIN_OP(T)                                    \
    T __min_##T(T lhs, T rhs) {                             \
        return min(lhs, rhs);                               \
    }

#define DEFINE_MAX_OP(T)                                    \
    T __max_##T(T lhs, T rhs) {                             \
        return max(lhs, rhs);                               \
    }

#define DEFINE_BF16ADD_OP(T)                                                   \
    T __add_##T(T lhs, T rhs) {                                            \
        return __fp32_to_bf16(__bf16_to_fp32(lhs) + __bf16_to_fp32(rhs));      \
    }

#define DEFINE_BF16MULT_OP(T)                                                  \
    T __mult_##T(T lhs, T rhs) {                                           \
        return __fp32_to_bf16(__bf16_to_fp32(lhs) * __bf16_to_fp32(rhs));      \
    }

#define DEFINE_BF16MIN_OP(T)                                                   \
    T __min_##T(T lhs, T rhs) {                                            \
        return __fp32_to_bf16(min(__bf16_to_fp32(lhs), __bf16_to_fp32(rhs)));  \
    }

#define DEFINE_BF16MAX_OP(T)                                                   \
    T __max_##T(T lhs, T rhs) {                                            \
        return __fp32_to_bf16(max(__bf16_to_fp32(lhs), __bf16_to_fp32(rhs)));  \
    }

#define DEFINE_OPS(T)                                       \
    DEFINE_ADD_OP(T)                                        \
    DEFINE_MULT_OP(T)                                       \
    DEFINE_MIN_OP(T)                                        \
    DEFINE_MAX_OP(T)

#define DEFINE_BF16OPS(T)                                   \
    DEFINE_BF16ADD_OP(T)                                    \
    DEFINE_BF16MULT_OP(T)                                   \
    DEFINE_BF16MIN_OP(T)                                    \
    DEFINE_BF16MAX_OP(T)

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

DEFINE_BF16OPS(ushort)
// TODO: Enable when half is supported
/*DEFINE_OPS(half)*/

// Define the actual kernels
DEFINE_KERNELS_WITH_OP(add)
DEFINE_KERNELS_WITH_OP(mult)
DEFINE_KERNELS_WITH_OP(min)
DEFINE_KERNELS_WITH_OP(max)

DEFINE_KERNELS_WITH_BF16OP(add)
DEFINE_KERNELS_WITH_BF16OP(mult)
DEFINE_KERNELS_WITH_BF16OP(min)
DEFINE_KERNELS_WITH_BF16OP(max)

// numa
// TODO: vecsize
#define DEFINE_KERNEL_NUMA(Name, T,  Op, OpName)                                                         \
__kernel void allreduce_execution_numa_##Name##_##OpName(size_t my_rank,                                         \
                                        size_t comm_size,                                                   \
                                        size_t elems_count,                                                 \
                                        const __global T* input_buffer,                                     \
                                        __global T* output_buffer,                                          \
                                                                                                            \
                                        __global T* tmp_buffer,                                             \
                                        __global volatile int* left_wrote_to_me_flag,                       \
                                        __global volatile int* i_ready_to_receive_flag,                     \
                                                                                                            \
                                        __global volatile int* local_barrier_flag,                          \
                                                                                                            \
                                        __global T* right_temp_buffer,                                      \
                                        __global volatile int* i_send_to_right_flag,                        \
                                        __global volatile int* right_ready_to_recv_flag) { \
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

DEFINE_KERNEL_NUMA(float32, float4, __add_float4, add)
DEFINE_KERNEL_NUMA(float64, double4, __add_double4, add)

// TODO: implement support for missing types
//DEFINE_KERNEL_NUMA(float16, half, 1)

DEFINE_KERNEL_NUMA(bfloa16, ushort, __add_ushort, add)
