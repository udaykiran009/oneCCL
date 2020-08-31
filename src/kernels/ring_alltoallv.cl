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

size_t get_left_rank(size_t rank, size_t comm_size) {
    return rank == 0 ? comm_size - 1 : rank - 1;
}

size_t get_right_rank(size_t rank, size_t comm_size) {
    return rank == (comm_size - 1) ? 0 : rank + 1;
}

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
__kernel void alltoallv_execution_float(size_t my_rank, //0
                                        size_t comm_size, //1

                                        __global size_t* send_elem_counts, //2
                                        __global size_t* send_elem_offsets, //3
                                        __global size_t* recv_elem_counts, //4
                                        __global size_t* recv_elem_offsets, //5

                                        const __global float4* input_buffer, //6
                                        __global float4* output_buffer, //7
                                        __global float4* tmp_buffer, //8
                                        __global float4* right_temp_buffer, //9

                                        __global volatile int* left_wrote_to_me_flag, //10
                                        __global volatile int* i_ready_to_receive_flag, //11
                                        __global volatile int* proxy_size_flag, //12
                                        __global volatile int* i_send_to_right_flag, //13
                                        __global volatile int* right_ready_to_recv_flag, //14
                                        __global volatile int* right_proxy_size_flag //15
) {
    size_t work_group_size = get_global_size(0);
    int thread_id = get_global_id(0);
    size_t work_rank = my_rank;

    int ready_to_recv_sync_count = 1;
    int can_send_sync_count = 1;

    int proxy_size = 0;
    size_t prev_segment_size = 0;
    size_t tmp_segment_offset = 0;

    int output_buffer_offset = 0;
    int counter = 0;
    int chunk_counter = 0;

    size_t segment_size = send_elem_counts[work_rank] / 4;
    size_t segment_offset = recv_elem_offsets[work_rank] / 4;
    size_t input_buffer_offset = send_elem_offsets[work_rank] / 4;

    //TODO: consider a new solving of calculating segment_offset
    /*for (size_t i = 0; i < work_rank; i++) {
        segment_offset = send_elem_counts[i] / 4;
    }*/

    for (size_t i = 0; i < segment_size; i += work_group_size) {
        output_buffer[segment_offset + thread_id + i] =
            input_buffer[input_buffer_offset + thread_id + i];
    }

    PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);
    WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);
    barrier(CLK_LOCAL_MEM_FENCE);

    for (size_t idx = 0; idx < comm_size - 1; idx++) {
        work_rank = get_right_rank(work_rank, comm_size);
        segment_size = send_elem_counts[work_rank] / 4;
        segment_offset = send_elem_offsets[work_rank] / 4;
        for (size_t i = 0; i < segment_size; i += work_group_size) {
            right_temp_buffer[tmp_segment_offset + thread_id + i] =
                input_buffer[segment_offset + thread_id + i];
        }
        tmp_segment_offset += segment_size;
        if (tmp_segment_offset >= segment_size * comm_size)
            tmp_segment_offset = 0;
        proxy_size += segment_size;
    }

    barrier(CLK_LOCAL_MEM_FENCE);
    SET_PROXY_SIZE(proxy_size);
    I_SENT(i_send_to_right_flag);
    WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count);
    barrier(CLK_LOCAL_MEM_FENCE);

    work_rank = my_rank;
    for (size_t iter = 0; iter < comm_size - 2; iter++) {
        work_rank = get_left_rank(work_rank, comm_size);
        segment_size = recv_elem_counts[work_rank] / 4;
        segment_offset = recv_elem_offsets[work_rank] / 4;

        for (size_t i = 0; i < segment_size; i += work_group_size) {
            output_buffer[segment_offset + thread_id + i] = tmp_buffer[thread_id + i];
        }
        prev_segment_size = segment_size;

        barrier(CLK_LOCAL_MEM_FENCE);
        GET_PROXY_SIZE(proxy_size);
        barrier(CLK_LOCAL_MEM_FENCE);

        proxy_size -= prev_segment_size;
        segment_size = proxy_size;

        for (size_t i = 0; i < segment_size; i += work_group_size) {
            right_temp_buffer[thread_id + i] = tmp_buffer[prev_segment_size + thread_id + i];
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);
        barrier(CLK_LOCAL_MEM_FENCE);

        SET_PROXY_SIZE(proxy_size);

        I_SENT(i_send_to_right_flag);
        WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count);
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    work_rank = get_left_rank(work_rank, comm_size);
    segment_size = recv_elem_counts[work_rank] / 4;
    segment_offset = recv_elem_offsets[work_rank] / 4;
    for (size_t i = 0; i < segment_size; i += work_group_size) {
        output_buffer[segment_offset + thread_id + i] = tmp_buffer[thread_id + i];
    }
    barrier(CLK_LOCAL_MEM_FENCE);
}

__kernel void alltoallv_execution_char(size_t my_rank,
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

__kernel void alltoallv_execution_int(size_t my_rank,
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
__kernel void alltoallv_execution_bfp16(size_t my_rank,
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

__kernel void alltoallv_execution_double(size_t my_rank,
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

__kernel void alltoallv_execution_int64_t(size_t my_rank,
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

__kernel void alltoallv_execution_uint64_t(size_t my_rank,
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
__kernel void alltoallv_execution_numa_char(size_t my_rank,
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

__kernel void alltoallv_execution_numa_int(size_t my_rank,
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

__kernel void alltoallv_execution_numa_bfp16(size_t my_rank,
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

__kernel void alltoallv_execution_numa_float(size_t my_rank,
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

__kernel void alltoallv_execution_numa_double(size_t my_rank,
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

__kernel void alltoallv_execution_numa_int64_t(size_t my_rank,
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

__kernel void alltoallv_execution_numa_uint64_t(size_t my_rank,
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
