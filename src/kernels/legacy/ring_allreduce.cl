#include "../common.h"
#include "../shared.h"

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
#define DEFINE_ALLREDUCE_KERNEL(Name, T, VecSize, Op, OpName) \
    __kernel void allreduce_kernel_##Name##_##OpName(int my_rank, \
                                                     int comm_size, \
                                                     ulong count, \
                                                     const __global T* input_buffer, \
                                                     __global T* output_buffer, \
                                                     const __global T* right_input_buffer, \
                                                     __global T* right_output_buffer) { \
        DEBUG_BLOCK(printf("rank: %d, comm size: %d, count: %zu\n", my_rank, comm_size, count)); \
        size_t work_group_size = get_global_size(0); \
        size_t thread_id = get_global_id(0); \
\
        for (size_t i = 0; thread_id + i < count; i += work_group_size) { \
            const size_t idx = thread_id + i; \
            output_buffer[idx] = Op(input_buffer[idx], right_input_buffer[idx]); \
            right_output_buffer[idx] = output_buffer[idx]; \
        } \
    }

#define DEFINE_REDUCE_LOCAL_KERNEL(Name, T, VecSize, Op, OpName) \
    __kernel void reduce_local_kernel_##Name##_##OpName(int my_rank, \
                                                        int comm_size, \
                                                        ulong count, \
                                                        const __global T* input_buffer_1, \
                                                        const __global T* input_buffer_2, \
                                                        __global T* output_buffer) { \
        DEBUG_BLOCK(printf("rank: %d, comm size: %d, count: %zu\n", my_rank, comm_size, count)); \
        size_t work_group_size = get_global_size(0); \
        size_t thread_id = get_global_id(0); \
\
        for (size_t i = 0; thread_id + i < count; i += work_group_size) { \
            const size_t idx = thread_id + i; \
            output_buffer[idx] = Op(input_buffer_1[idx], input_buffer_2[idx]); \
        } \
    }

#define DEFINE_KERNEL_MONOLITHIC(Name, T, VecSize, Op, OpName) \
    __kernel void allreduce_execution_monolithic##Name##_##OpName( \
        int my_rank, \
        int comm_size, \
        ulong elems_count, \
        const __global T* input_buffer, \
        __global T* output_buffer, \
\
        __global T* tmp_buffer, \
        __global sync_flag_type* left_wrote_to_me_flag, \
        __global sync_flag_type* i_ready_to_receive_flag, \
\
        __global volatile int* local_barrier_flag, \
\
        __global T* right_temp_buffer, \
        __global sync_flag_type* i_send_to_right_flag, \
        __global sync_flag_type* right_ready_to_recv_flag) { \
        /*Known limitation                                                                                      \
      1) TBA: double-buffering                                                                              \
      2) TBA: chunking                                                                                      \
      3) TBA: multiple WGs; */ \
        elems_count = elems_count / VecSize; /*reduce by vectro T */ \
        /* The algorithm is working on segments of data of size = segment_size - 1 for each rank, e.g if there are \
           4 ranks, the data for each of them is split into 4 segments. It's possible that the last segment is smaller \
           than the others in case if elems_count % comm_size != 0, This case is handled by adding a constraint for \
           all operation on the segments, i.e. reads and writes. This is that offset + thread_id + i < elems_count \
           checks below are for. \
           Note: we only check offsets of input and output buffers(e.g. segment_offset) in the loop constraint. \
           For tmp buffer we allocate and use at most 2 segments so its offset is always <= offset of input/output buffers \
           therefore no need to introduce additional check */ \
        size_t segment_size = ring_allreduce_get_segment_size(elems_count, comm_size); \
        size_t work_group_size = get_global_size(0); \
        int my_rank_buffer_start_idx = my_rank * segment_size; \
        int thread_id = get_global_id(0); \
\
        int work_rank = my_rank; \
        int ready_to_recv_sync_count = 1; \
        int can_send_sync_count = 1; \
\
        size_t tmp_buffer_offset = 0; \
        size_t right_tmp_buffer_offset = segment_size; \
\
        DEBUG_BLOCK(/*int sg_id = get_sub_group_id();*/ \
                    printf("kernel %d.%d work_group_size: %zu, segment_size: %zu\n", \
                           my_rank, \
                           thread_id, \
                           work_group_size, \
                           segment_size /*, sg_id*/)); \
\
        /*1. copy own part to the right rank*/ \
        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
        /*aka send*/ \
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
        /* As we do operations on segments in parallel and with step size = work_group_size, if segment_size % work_group_size \
           we have a remainder such that some workitems still have a loop iteration to do, but there is no more data, we need \
           to stop them earlyer by checking for thread_id + i < segment_size */ \
        for (size_t i = 0; thread_id + i < segment_size && \
                           my_rank_buffer_start_idx + thread_id + i < elems_count; \
             i += work_group_size) { \
            right_temp_buffer[thread_id + i] = \
                input_buffer[my_rank_buffer_start_idx + thread_id + i]; \
        } \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
        I_SENT(i_send_to_right_flag); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
        /*aka receive*/ \
        WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
        DEBUG_BLOCK(printf("kernel %d.%d send complete\n", my_rank, thread_id)); \
        /*2nd phase - reduce scatter                                                                            \
          On this phase we do a reduce scatter operation among running ranks and after it we'll have \
          1 correctly reduced(i.e. reduced from every rank) segment for each rank on position i + 1 for rank i \
          of output buffer (e.g. comm_size == 3 for rank 0 we'll have 1st segment reduced, for 1st - 2nd and for 3rd - 0th) */ \
\
        /* Get data written by left rank to our temp buffer, reduce with our part and send to right rank.
            Repeat until we get the reduced data from each rank */ \
        for (int iter_idx = 0; iter_idx < comm_size - 2; ++iter_idx) { \
            work_rank = get_left_rank(work_rank, comm_size); \
            size_t segment_offset = work_rank * segment_size; \
\
            /*left rank has written data to our temp buffer, reduce it with corresponding element               \
              from our initial buffer and send to the right rank  */ \
\
            PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
            /*aka send*/ \
            WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
            for (size_t i = 0; \
                 thread_id + i < segment_size && thread_id + i + segment_offset < elems_count; \
                 i += work_group_size) { \
                DEBUG_BLOCK( \
                    printf("kernel %d.%d, phase 2.%d -- temp[%zu] = " FORMAT##_##T \
                           ", this[%zu] = " FORMAT##_##T "\n", \
                           my_rank, \
                           thread_id, \
                           iter_idx, \
                           segment_offset + thread_id + i, \
                           ELEMENTS##_##VecSize(tmp_buffer[thread_id + i]), \
                           segment_offset + thread_id + i, \
                           ELEMENTS##_##VecSize(input_buffer[segment_offset + thread_id + i]))); \
                right_temp_buffer[thread_id + i + right_tmp_buffer_offset] = \
                    Op(tmp_buffer[thread_id + i + tmp_buffer_offset], \
                       input_buffer[segment_offset + thread_id + i]); \
            } \
            barrier(CLK_GLOBAL_MEM_FENCE); \
            I_SENT(i_send_to_right_flag); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
            /*aka receive*/ \
            WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
            SWAP_VARIABLES(tmp_buffer_offset, right_tmp_buffer_offset, size_t); \
        } \
\
        DEBUG_BLOCK(printf( \
            "kernel %d.%d, phase 2 completed, work_rank %d\n", my_rank, thread_id, work_rank)); \
\
        /* Local reduction */ \
        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
        /*aka send*/ \
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
        work_rank = get_left_rank(work_rank, comm_size); \
        size_t segment_offset = work_rank * segment_size; \
\
        /*left rank has written data to our temp buffer, reduce it with corresponding element from our initial buffer \
          and put to output buffer*/ \
\
        for (size_t i = 0; \
             thread_id + i < segment_size && segment_offset + thread_id + i < elems_count; \
             i += work_group_size) { \
            /* TODO: use private variable to store result, but must check whether this improves perf*/ \
            output_buffer[segment_offset + thread_id + i] = \
                Op(tmp_buffer[thread_id + i + tmp_buffer_offset], \
                   input_buffer[segment_offset + thread_id + i]); \
            right_temp_buffer[thread_id + i + right_tmp_buffer_offset] = \
                output_buffer[segment_offset + thread_id + i]; \
            /* We should be able to issue 2 stores for the same data - local and remote,                        \
            no need for local read */ \
        } \
        barrier(CLK_GLOBAL_MEM_FENCE); \
        I_SENT(i_send_to_right_flag); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
        /*aka receive*/ \
        WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
        SWAP_VARIABLES(tmp_buffer_offset, right_tmp_buffer_offset, size_t); \
\
        DEBUG_BLOCK(printf( \
            "kernel %d.%d, phase 2 completed, work_rank %d\n", my_rank, thread_id, work_rank)); \
\
        work_rank = my_rank; \
        /*3rd phase - allgather                                                                                 \
          Copy ready data from temp buffer by [segment_offset] offset, reduce and send data by [work_rank] offset */ \
        for (int iter_idx = 0; iter_idx < comm_size - 2; ++iter_idx) { \
            segment_offset = work_rank * segment_size; \
            /*copy reduced value to initial buffer*/ \
\
            PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
            /*aka send*/ \
            WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
            for (size_t i = 0; \
                 thread_id + i < segment_size && segment_offset + thread_id + i < elems_count; \
                 i += work_group_size) { \
                /* TODO: use private variable to store tmp, but must check whether this improves perf*/ \
                output_buffer[segment_offset + thread_id + i] = \
                    tmp_buffer[thread_id + i + tmp_buffer_offset]; \
\
                DEBUG_BLOCK(printf("kernel %d.%d, phase 3.%d -- send " FORMAT##_##T \
                                   " to idx %zu, rank %zu\n", \
                                   my_rank, \
                                   thread_id, \
                                   iter_idx, \
                                   ELEMENTS##_##VecSize(tmp_buffer[thread_id + i]), \
                                   segment_offset + thread_id, \
                                   work_rank + i)); \
                right_temp_buffer[thread_id + i + right_tmp_buffer_offset] = \
                    tmp_buffer[thread_id + i + tmp_buffer_offset]; \
            } \
            barrier(CLK_GLOBAL_MEM_FENCE); \
            I_SENT(i_send_to_right_flag); \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
            /*aka receive*/ \
            WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count); \
\
            work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
            work_rank = get_left_rank(work_rank, comm_size); \
\
            SWAP_VARIABLES(tmp_buffer_offset, right_tmp_buffer_offset, size_t); \
        } \
\
        /* Copy from tmp to output buf*/ \
        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_all_svm_devices); \
\
        /*aka send*/ \
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count); \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE); \
\
        segment_offset = work_rank * segment_size; \
        for (size_t i = 0; \
             (i + thread_id) < segment_size && segment_offset + thread_id + i < elems_count; \
             i += work_group_size) { \
            output_buffer[segment_offset + thread_id + i] = \
                tmp_buffer[thread_id + i + tmp_buffer_offset]; \
        } \
\
        DEBUG_BLOCK(printf("kernel %d.%d completed\n", my_rank, thread_id)); \
    }

// Define kernels for a specific operation for all the supported types.
// Note: for op function we use convention __<OpName>_<type>, where type is the actual type(e.g. int4, float)
// FIXME: Temporary use scalar types instead of vector ones. This is a workaround for issues in case when
// elems_count % VecSize != 0. Need to find a proper fix with a good performance.
#define VEC_SIZE RING_ALLREDUCE_VEC_SIZE

#define DEFINE_KERNELS_WITH_OP(OpName) \
    DEFINE_ALLREDUCE_KERNEL(int8, char, VEC_SIZE, __##OpName##_##char, OpName) \
    DEFINE_ALLREDUCE_KERNEL(uint8, uchar, VEC_SIZE, __##OpName##_##uchar, OpName) \
\
    DEFINE_ALLREDUCE_KERNEL(int16, short, VEC_SIZE, __##OpName##_##short, OpName) \
    DEFINE_ALLREDUCE_KERNEL(uint16, ushort, VEC_SIZE, __##OpName##_##ushort, OpName) \
\
    DEFINE_ALLREDUCE_KERNEL(int32, int, VEC_SIZE, __##OpName##_##int, OpName) \
    DEFINE_ALLREDUCE_KERNEL(uint32, uint, VEC_SIZE, __##OpName##_##uint, OpName) \
\
    DEFINE_ALLREDUCE_KERNEL(int64, long, VEC_SIZE, __##OpName##_##long, OpName) \
    DEFINE_ALLREDUCE_KERNEL(uint64, ulong, VEC_SIZE, __##OpName##_##ulong, OpName) \
\
    DEFINE_ALLREDUCE_KERNEL(float32, float, VEC_SIZE, __##OpName##_##float, OpName) \
    DEFINE_ALLREDUCE_KERNEL(float64, double, VEC_SIZE, __##OpName##_##double, OpName)

#define DEFINE_KERNELS_WITH_LP_OP(OpName) \
    DEFINE_ALLREDUCE_KERNEL(bfloat16, ushort, VEC_SIZE, __bf16_##OpName##_##ushort, OpName) \
    DEFINE_ALLREDUCE_KERNEL(float16, half, VEC_SIZE, __##OpName##_##half, OpName)

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
DEFINE_OPS(char)
DEFINE_OPS(uchar)

DEFINE_OPS(short)
DEFINE_OPS(ushort)

DEFINE_OPS(int)
DEFINE_OPS(uint)

DEFINE_OPS(long)
DEFINE_OPS(ulong)

DEFINE_OPS(float)
DEFINE_OPS(double)

DEFINE_BF16OPS(ushort)
DEFINE_FP16OPS(half)

// Define the actual kernels
DEFINE_KERNELS_WITH_OP(sum)
DEFINE_KERNELS_WITH_OP(prod)
DEFINE_KERNELS_WITH_OP(min)
DEFINE_KERNELS_WITH_OP(max)

DEFINE_KERNELS_WITH_LP_OP(sum)
DEFINE_KERNELS_WITH_LP_OP(prod)
DEFINE_KERNELS_WITH_LP_OP(min)
DEFINE_KERNELS_WITH_LP_OP(max)

#define DEFINE_REDUCE_LOCAL_KERNELS_WITH_OP(OpName) \
    DEFINE_REDUCE_LOCAL_KERNEL(int8, char, VEC_SIZE, __reduce_local_##OpName##_##char, OpName) \
    DEFINE_REDUCE_LOCAL_KERNEL(uint8, uchar, VEC_SIZE, __reduce_local_##OpName##_##uchar, OpName) \
\
    DEFINE_REDUCE_LOCAL_KERNEL(int16, short, VEC_SIZE, __reduce_local_##OpName##_##short, OpName) \
    DEFINE_REDUCE_LOCAL_KERNEL( \
        uint16, ushort, VEC_SIZE, __reduce_local_##OpName##_##ushort, OpName) \
\
    DEFINE_REDUCE_LOCAL_KERNEL(int32, int, VEC_SIZE, __reduce_local_##OpName##_##int, OpName) \
    DEFINE_REDUCE_LOCAL_KERNEL(uint32, uint, VEC_SIZE, __reduce_local_##OpName##_##uint, OpName) \
\
    DEFINE_REDUCE_LOCAL_KERNEL(int64, long, VEC_SIZE, __reduce_local_##OpName##_##long, OpName) \
    DEFINE_REDUCE_LOCAL_KERNEL(uint64, ulong, VEC_SIZE, __reduce_local_##OpName##_##ulong, OpName) \
\
    DEFINE_REDUCE_LOCAL_KERNEL( \
        float32, float, VEC_SIZE, __reduce_local_##OpName##_##float, OpName) \
    DEFINE_REDUCE_LOCAL_KERNEL( \
        float64, double, VEC_SIZE, __reduce_local_##OpName##_##double, OpName)

#define DEFINE_KERNELS_REDUCE_LOCAL_WITH_LP_OP(OpName) \
    DEFINE_REDUCE_LOCAL_KERNEL( \
        bfloat16, ushort, VEC_SIZE, __reduce_local_bf16_##OpName##_##ushort, OpName) \
    DEFINE_REDUCE_LOCAL_KERNEL(float16, half, VEC_SIZE, __reduce_local_##OpName##_##half, OpName)

#define DEFINE_REDUCE_LOCAL_OPS(T) \
    DEFINE_REDUCE_LOCAL_SUM_OP(T) \
    DEFINE_REDUCE_LOCAL_PROD_OP(T) \
    DEFINE_REDUCE_LOCAL_MIN_OP(T) \
    DEFINE_REDUCE_LOCAL_MAX_OP(T)

#define DEFINE_REDUCE_LOCAL_BF16OPS(T) \
    DEFINE_REDUCE_LOCAL_BF16SUM_OP(T) \
    DEFINE_REDUCE_LOCAL_BF16PROD_OP(T) \
    DEFINE_REDUCE_LOCAL_BF16MIN_OP(T) \
    DEFINE_REDUCE_LOCAL_BF16MAX_OP(T)

#define DEFINE_REDUCE_LOCAL_FP16OPS(T) \
    DEFINE_REDUCE_LOCAL_FP16SUM_OP(T) \
    DEFINE_REDUCE_LOCAL_FP16PROD_OP(T) \
    DEFINE_REDUCE_LOCAL_FP16MIN_OP(T) \
    DEFINE_REDUCE_LOCAL_FP16MAX_OP(T)

// Define Op function for each supported type(use vector types for some of them as required by the kernel)
DEFINE_REDUCE_LOCAL_OPS(char)
DEFINE_REDUCE_LOCAL_OPS(uchar)

DEFINE_REDUCE_LOCAL_OPS(short)
DEFINE_REDUCE_LOCAL_OPS(ushort)

DEFINE_REDUCE_LOCAL_OPS(int)
DEFINE_REDUCE_LOCAL_OPS(uint)

DEFINE_REDUCE_LOCAL_OPS(long)
DEFINE_REDUCE_LOCAL_OPS(ulong)

DEFINE_REDUCE_LOCAL_OPS(float)
DEFINE_REDUCE_LOCAL_OPS(double)

DEFINE_REDUCE_LOCAL_BF16OPS(ushort)
DEFINE_REDUCE_LOCAL_FP16OPS(half)

// Define the actual kernels
DEFINE_REDUCE_LOCAL_KERNELS_WITH_OP(sum)
DEFINE_REDUCE_LOCAL_KERNELS_WITH_OP(prod)
DEFINE_REDUCE_LOCAL_KERNELS_WITH_OP(min)
DEFINE_REDUCE_LOCAL_KERNELS_WITH_OP(max)

DEFINE_KERNELS_REDUCE_LOCAL_WITH_LP_OP(sum)
DEFINE_KERNELS_REDUCE_LOCAL_WITH_LP_OP(prod)
DEFINE_KERNELS_REDUCE_LOCAL_WITH_LP_OP(min)
DEFINE_KERNELS_REDUCE_LOCAL_WITH_LP_OP(max)
