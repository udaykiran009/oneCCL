#pragma OPENCL EXTENSION cl_intel_subgroups : enable
#pragma OPENCL EXTENSION cl_khr_subgroups : enable

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

#define WAIT_SIGNAL_TO_SEND(_sync_flag, _desired) \
    if (thread_id == 0) { \
        LOG_OUTGOING_DATA_START(my_rank); \
        while (_desired != *_sync_flag) { \
        }; \
        LOG_OUTGOING_DATA_END(my_rank); \
        ++_desired; \
    }

#define I_SENT(_sync_flag) \
    if (thread_id == 0) { \
        (*_sync_flag)++; \
    }

/**
 * @param left_wrote_to_me_flag  - located in the memory of the current kernel, left rank uses a pointer to it to notify that he has sent some data.
 * @param i_ready_to_receive_flag - located in the memory of the left peer, left rank uses a pointer to it to check if he can send some data to us
 * @param i_send_to_right_flag - located in the memory of the right kernel. Used by current kernel to notify right kernel that he has sent some data
 * @param right_ready_to_recv_flag - located in the memory of the current kernel. Used by right kernel to notify us it's ready to receive
 */
__kernel void reduce_execution_float(size_t my_rank, //0
                                     size_t comm_size, //1
                                     size_t elems_count, //2
                                     const __global float4* input_buffer, //3
                                     __global float4* output_buffer, //4

                                     __global float4* tmp_buffer, //5
                                     __global volatile int* left_wrote_to_me_flag, //6
                                     __global volatile int* i_ready_to_receive_flag, //7

                                     __global volatile int* local_barrier_flag, //8

                                     __global float4* right_temp_buffer, //9
                                     __global volatile int* i_send_to_right_flag, //10
                                     __global volatile int* right_ready_to_recv_flag, //11
                                     size_t root //12
) {
    // The RING based algorithm,
    // where the root rank is the end point.
    // The direction is from Left -> to Right
    //
    //                      0 (in-between rank)
    //                     / \
    // (the farthest rank)3   1 (in-between rank)
    //    from Root        \ /
    //                      2 (Root.End point)

    // Root(2) - consumer
    // In-between ranks(0,1) - consumer producer
    // The farthest rank(3) - producer

    elems_count = elems_count / 4;
    size_t work_group_size = get_global_size(0);
    size_t segment_count = elems_count / work_group_size;
    int thread_id = get_global_id(0);

    int ready_to_recv_sync_count = 1;
    int can_send_sync_count = 1;

#ifdef KERNEL_DEBUG
    printf("kernel %zu.%d work_group_size: %d\n", my_rank, thread_id, work_group_size);
#endif

    if (my_rank == root) { //consumer r:ROOT
        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);
        /*
            TODO consider prefetch v
            oid prefetch(const __global gentype *p, size_t num_gentypes)
        */
        WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count);
        barrier(CLK_LOCAL_MEM_FENCE);
        for (size_t i = 0; i < segment_count; i++) {
#ifdef KERNEL_DEBUG
            printf("kernel %zu.%d, phase 2. -- temp[%zu] = %f, this[%zu] = %f\n",
                   my_rank,
                   thread_id,
                   i + thread_id,
                   tmp_buffer[thread_id + i],
                   i + thread_id,
                   input_buffer[i + thread_id]);
#endif
            output_buffer[work_group_size * i + thread_id] =
                input_buffer[work_group_size * i + thread_id] +
                tmp_buffer[work_group_size * i + thread_id];
        }
        barrier(CLK_GLOBAL_MEM_FENCE);
    }
    else if (my_rank == (root + 1) % comm_size) { // producer (the farthest rank)
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);
        barrier(CLK_LOCAL_MEM_FENCE);

        for (size_t i = 0; i < segment_count; i++) {
            right_temp_buffer[work_group_size * i + thread_id] =
                input_buffer[work_group_size * i + thread_id];
        }

        barrier(CLK_GLOBAL_MEM_FENCE);
        I_SENT(i_send_to_right_flag);
    }
    else { // consumer producer (in-between ranks)
        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);
        /*
            TODO consider prefetch v
            oid prefetch(const __global gentype *p, size_t num_gentypes)
        */
        WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count);
        barrier(CLK_LOCAL_MEM_FENCE);

        for (size_t i = 0; i < segment_count; i++) {
            right_temp_buffer[work_group_size * i + thread_id] =
                tmp_buffer[work_group_size * i + thread_id] +
                input_buffer[work_group_size * i + thread_id];
        }

        barrier(CLK_GLOBAL_MEM_FENCE);
        I_SENT(i_send_to_right_flag);
    }

#ifdef KERNEL_DEBUG
    printf("kernel %zu.%d completed\n", my_rank, thread_id);
#endif
}

__kernel void reduce_execution_char(size_t my_rank,
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

__kernel void reduce_execution_int(size_t my_rank,
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
typedef ushort bf16;
__kernel void reduce_execution_bf16(size_t my_rank,
                                     size_t comm_size,
                                     size_t elems_count,
                                     const __global bf16* input_buffer,
                                     __global bf16* output_buffer,

                                     __global bf16* tmp_buffer,
                                     __global volatile int* left_wrote_to_me_flag,
                                     __global volatile int* i_ready_to_receive_flag,

                                     __global volatile int* local_barrier_flag,

                                     __global bf16* right_temp_buffer,
                                     __global volatile int* i_send_to_right_flag,
                                     __global volatile int* right_ready_to_recv_flag) {
    return;
}

__kernel void reduce_execution_double(size_t my_rank,
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

__kernel void reduce_execution_int64_t(size_t my_rank,
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

__kernel void reduce_execution_uint64_t(size_t my_rank,
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
__kernel void reduce_execution_numa_char(size_t my_rank,
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

__kernel void reduce_execution_numa_int(size_t my_rank,
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

__kernel void reduce_execution_numa_bf16(size_t my_rank,
                                          size_t comm_size,
                                          size_t elems_count,
                                          const __global bf16* input_buffer,
                                          __global bf16* output_buffer,

                                          __global bf16* tmp_buffer,
                                          __global volatile int* left_wrote_to_me_flag,
                                          __global volatile int* i_ready_to_receive_flag,

                                          __global volatile int* local_barrier_flag,

                                          __global bf16* right_temp_buffer,
                                          __global volatile int* i_send_to_right_flag,
                                          __global volatile int* right_ready_to_recv_flag) {
    return;
}

__kernel void reduce_execution_numa_float(size_t my_rank,
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

__kernel void reduce_execution_numa_double(size_t my_rank,
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

__kernel void reduce_execution_numa_int64_t(size_t my_rank,
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

__kernel void reduce_execution_numa_uint64_t(size_t my_rank,
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
