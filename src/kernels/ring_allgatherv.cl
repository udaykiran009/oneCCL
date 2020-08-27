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
__kernel void allgatherv_execution_float(size_t my_rank,
                                         size_t comm_size,

                                         __global size_t* recv_elem_counts,
                                         __global size_t* recv_elem_offsets,

                                         const __global float4* input_buffer,
                                         __global float4* output_buffer,
                                         __global float4* right_output_buffer,

                                         __global volatile int* left_wrote_to_me_flag,
                                         __global volatile int* i_ready_to_receive_flag,
                                         __global volatile int* i_send_to_right_flag,
                                         __global volatile int* right_ready_to_recv_flag) {
    //Known limitation
    //1) MUST: elems_count >= get_global_size(0) * 4 (vector size) * comm_size
    //2) MUST: elems_count be aligned with 'get_global_size(0) * 4 (vector size)'
    //3) TBA: double-buffering
    //4) TBA: chunking
    //5) TBA: multiple WGs;

    size_t work_rank = my_rank;
    size_t segment_size = recv_elem_counts[work_rank] / 4; //reduce by vectro float4
    size_t segment_offset = recv_elem_offsets[work_rank] / 4;

    size_t work_group_size = get_global_size(0);
    int thread_id = get_global_id(0);

    int ready_to_recv_sync_count = 1;
    int can_send_sync_count = 1;

#ifdef KERNEL_DEBUG
    //int sg_id = get_sub_group_id();
    printf("kernel %zu.%d work_group_size: %d, segment_size: %d\n",
           my_rank,
           thread_id,
           work_group_size,
           segment_size /*, sg_id*/);
#endif

    // 1. copy own data to the right rank
    PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);
    WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);
    barrier(CLK_LOCAL_MEM_FENCE);

    for (size_t i = 0; i < segment_size; i += work_group_size) {
        right_output_buffer[thread_id + i + segment_offset] = input_buffer[thread_id + i];
    }
    barrier(CLK_GLOBAL_MEM_FENCE);

    I_SENT(i_send_to_right_flag);
    WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count);
    barrier(CLK_LOCAL_MEM_FENCE);

#ifdef KERNEL_DEBUG
    printf("kernel %zu.%d send complete\n", my_rank, thread_id);
#endif

    for (size_t iter_idx = 0; iter_idx < comm_size - 2; ++iter_idx) {
        work_rank = get_left_rank(work_rank, comm_size);

        segment_size = recv_elem_counts[work_rank] / 4;
        segment_offset = recv_elem_offsets[work_rank] / 4;

        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);
        barrier(CLK_LOCAL_MEM_FENCE);

        for (size_t i = 0; i < segment_size; i += work_group_size) {
            right_output_buffer[thread_id + i + segment_offset] =
                output_buffer[thread_id + i + segment_offset];
        }
        barrier(CLK_GLOBAL_MEM_FENCE);

        I_SENT(i_send_to_right_flag);
        WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count);
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    work_rank = my_rank;
    segment_size = recv_elem_counts[work_rank] / 4;
    segment_offset = recv_elem_offsets[work_rank] / 4;

    for (size_t i = 0; i < segment_size; i += work_group_size) {
        output_buffer[thread_id + i + segment_offset] = input_buffer[thread_id + i];
    }
    barrier(CLK_GLOBAL_MEM_FENCE);

#ifdef KERNEL_DEBUG
    printf("kernel %zu.%d completed\n", my_rank, thread_id);
#endif
}

__kernel void allgatherv_execution_char(size_t my_rank,
                                        size_t comm_size,
                                        size_t elems_count,
                                        const __global char4* input_buffer,
                                        __global char4* output_buffer,

                                        __global char4* tmp_buffer,
                                        __global volatile int* left_wrote_to_me_flag,
                                        __global volatile int* i_ready_to_receive_flag,

                                        __global volatile int* local_barrier_flag,

                                        __global char4* right_temp_buffer,
                                        __global volatile int* i_send_to_right_flag,
                                        __global volatile int* right_ready_to_recv_flag) {
    return;
}

__kernel void allgatherv_execution_int(size_t my_rank,
                                       size_t comm_size,
                                       size_t elems_count,
                                       const __global int4* input_buffer,
                                       __global int4* output_buffer,

                                       __global int4* tmp_buffer,
                                       __global volatile int* left_wrote_to_me_flag,
                                       __global volatile int* i_ready_to_receive_flag,

                                       __global volatile int* local_barrier_flag,

                                       __global int4* right_temp_buffer,
                                       __global volatile int* i_send_to_right_flag,
                                       __global volatile int* right_ready_to_recv_flag) {
    return;
}

//TODO
typedef ushort bfp16;
__kernel void allgatherv_execution_bfp16(size_t my_rank,
                                         size_t comm_size,
                                         size_t elems_count,
                                         const __global bfp16* input_buffer,
                                         __global bfp16* output_buffer,

                                         __global bfp16* tmp_buffer,
                                         __global volatile int* left_wrote_to_me_flag,
                                         __global volatile int* i_ready_to_receive_flag,

                                         __global volatile int* local_barrier_flag,

                                         __global bfp16* right_temp_buffer,
                                         __global volatile int* i_send_to_right_flag,
                                         __global volatile int* right_ready_to_recv_flag) {
    return;
}

__kernel void allgatherv_execution_double(size_t my_rank,
                                          size_t comm_size,
                                          size_t elems_count,
                                          const __global double4* input_buffer,
                                          __global double4* output_buffer,

                                          __global double4* tmp_buffer,
                                          __global volatile int* left_wrote_to_me_flag,
                                          __global volatile int* i_ready_to_receive_flag,

                                          __global volatile int* local_barrier_flag,

                                          __global double4* right_temp_buffer,
                                          __global volatile int* i_send_to_right_flag,
                                          __global volatile int* right_ready_to_recv_flag) {
    return;
}

__kernel void allgatherv_execution_int64_t(size_t my_rank,
                                           size_t comm_size,
                                           size_t elems_count,
                                           const __global long4* input_buffer,
                                           __global long4* output_buffer,

                                           __global long4* tmp_buffer,
                                           __global volatile int* left_wrote_to_me_flag,
                                           __global volatile int* i_ready_to_receive_flag,

                                           __global volatile int* local_barrier_flag,

                                           __global long4* right_temp_buffer,
                                           __global volatile int* i_send_to_right_flag,
                                           __global volatile int* right_ready_to_recv_flag) {
    return;
}

__kernel void allgatherv_execution_uint64_t(size_t my_rank,
                                            size_t comm_size,
                                            size_t elems_count,
                                            const __global ulong4* input_buffer,
                                            __global ulong4* output_buffer,

                                            __global ulong4* tmp_buffer,
                                            __global volatile int* left_wrote_to_me_flag,
                                            __global volatile int* i_ready_to_receive_flag,

                                            __global volatile int* local_barrier_flag,

                                            __global ulong4* right_temp_buffer,
                                            __global volatile int* i_send_to_right_flag,
                                            __global volatile int* right_ready_to_recv_flag) {
    return;
}

// numa
__kernel void allgatherv_execution_numa_char(size_t my_rank,
                                             size_t comm_size,
                                             size_t elems_count,
                                             const __global char4* input_buffer,
                                             __global char4* output_buffer,

                                             __global char4* tmp_buffer,
                                             __global volatile int* left_wrote_to_me_flag,
                                             __global volatile int* i_ready_to_receive_flag,

                                             __global volatile int* local_barrier_flag,

                                             __global char4* right_temp_buffer,
                                             __global volatile int* i_send_to_right_flag,
                                             __global volatile int* right_ready_to_recv_flag) {
    return;
}

__kernel void allgatherv_execution_numa_int(size_t my_rank,
                                            size_t comm_size,
                                            size_t elems_count,
                                            const __global int4* input_buffer,
                                            __global int4* output_buffer,

                                            __global int4* tmp_buffer,
                                            __global volatile int* left_wrote_to_me_flag,
                                            __global volatile int* i_ready_to_receive_flag,

                                            __global volatile int* local_barrier_flag,

                                            __global int4* right_temp_buffer,
                                            __global volatile int* i_send_to_right_flag,
                                            __global volatile int* right_ready_to_recv_flag) {
    return;
}

__kernel void allgatherv_execution_numa_bfp16(size_t my_rank,
                                              size_t comm_size,
                                              size_t elems_count,
                                              const __global bfp16* input_buffer,
                                              __global bfp16* output_buffer,

                                              __global bfp16* tmp_buffer,
                                              __global volatile int* left_wrote_to_me_flag,
                                              __global volatile int* i_ready_to_receive_flag,

                                              __global volatile int* local_barrier_flag,

                                              __global bfp16* right_temp_buffer,
                                              __global volatile int* i_send_to_right_flag,
                                              __global volatile int* right_ready_to_recv_flag) {
    return;
}

__kernel void allgatherv_execution_numa_float(size_t my_rank,
                                              size_t comm_size,
                                              size_t elems_count,
                                              const __global float4* input_buffer,
                                              __global float4* output_buffer,

                                              __global float4* tmp_buffer,
                                              __global volatile int* left_wrote_to_me_flag,
                                              __global volatile int* i_ready_to_receive_flag,

                                              __global volatile int* local_barrier_flag,

                                              __global float4* right_temp_buffer,
                                              __global volatile int* i_send_to_right_flag,
                                              __global volatile int* right_ready_to_recv_flag) {
    return;
}

__kernel void allgatherv_execution_numa_double(size_t my_rank,
                                               size_t comm_size,
                                               size_t elems_count,
                                               const __global double4* input_buffer,
                                               __global double4* output_buffer,

                                               __global double4* tmp_buffer,
                                               __global volatile int* left_wrote_to_me_flag,
                                               __global volatile int* i_ready_to_receive_flag,

                                               __global volatile int* local_barrier_flag,

                                               __global double4* right_temp_buffer,
                                               __global volatile int* i_send_to_right_flag,
                                               __global volatile int* right_ready_to_recv_flag) {
    return;
}

__kernel void allgatherv_execution_numa_int64_t(size_t my_rank,
                                                size_t comm_size,
                                                size_t elems_count,
                                                const __global long4* input_buffer,
                                                __global long4* output_buffer,

                                                __global long4* tmp_buffer,
                                                __global volatile int* left_wrote_to_me_flag,
                                                __global volatile int* i_ready_to_receive_flag,

                                                __global volatile int* local_barrier_flag,

                                                __global long4* right_temp_buffer,
                                                __global volatile int* i_send_to_right_flag,
                                                __global volatile int* right_ready_to_recv_flag) {
    return;
}

__kernel void allgatherv_execution_numa_uint64_t(size_t my_rank,
                                                 size_t comm_size,
                                                 size_t elems_count,
                                                 const __global ulong4* input_buffer,
                                                 __global ulong4* output_buffer,

                                                 __global ulong4* tmp_buffer,
                                                 __global volatile int* left_wrote_to_me_flag,
                                                 __global volatile int* i_ready_to_receive_flag,

                                                 __global volatile int* local_barrier_flag,

                                                 __global ulong4* right_temp_buffer,
                                                 __global volatile int* i_send_to_right_flag,
                                                 __global volatile int* right_ready_to_recv_flag) {
    return;
}
