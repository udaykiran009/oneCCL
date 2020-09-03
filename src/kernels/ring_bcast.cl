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

/**
 * @param left_wrote_to_me_flag  - located in the memory of the current kernel, left rank uses a pointer to it to notify that he has sent some data.
 * @param i_ready_to_receive_flag - located in the memory of the left peer, left rank uses a pointer to it to check if he can send some data to us
 * @param i_send_to_right_flag - located in the memory of the right kernel. Used by current kernel to notify right kernel that he has sent some data
 * @param right_ready_to_recv_flag - located in the memory of the current kernel. Used by right kernel to notify us it's ready to receive
 */

__kernel void bcast_execution_float(size_t my_rank, //0
                                    size_t comm_size, //1
                                    size_t elems_count, //2

                                    // const __global float4* input_buffer, //3
                                    // __global float4* output_buffer, //4
                                    __global float4* buffer, //3

                                    __global volatile int* left_wrote_to_me_flag, //5
                                    __global volatile int* i_ready_to_receive_flag, //6

                                    __global volatile int* local_barrier_flag, //7

                                    __global float4* right_buffer, //8
                                    __global volatile int* i_send_to_right_flag, //9
                                    __global volatile int* right_ready_to_recv_flag, //10
                                    size_t root //11
) {
    elems_count = elems_count / 4; //bcast by vector float4
    size_t work_group_size = get_global_size(0);
    size_t last_rank = root ? root - 1 : comm_size - 1;
    size_t segment_count = elems_count / work_group_size;
    int work_item_id = get_global_id(0);

    int ready_to_recv_sync_count = 1;
    int can_send_sync_count = 1;

#ifdef KERNEL_DEBUG
    printf("kernel %zu.%d work_group_size: %d, elems_count: %zu, segment_count %zu\n",
           my_rank,
           work_item_id,
           work_group_size,
           elems_count,
           segment_count);
#endif

    if (my_rank == root) {
        WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);
        barrier(CLK_LOCAL_MEM_FENCE);

        for (size_t i = 0; i < segment_count; i++) {
            right_buffer[work_group_size * i + work_item_id] =
                buffer[work_group_size * i + work_item_id];
            // input_buffer[work_group_size * i + work_item_id];
        }
        barrier(CLK_GLOBAL_MEM_FENCE);

        I_SENT(i_send_to_right_flag);

        // for (size_t i = 0; i < segment_count; i++) {
        //     output_buffer[work_group_size * i + work_item_id] =
        //         input_buffer[work_group_size * i + work_item_id];
        // }
        // barrier(CLK_GLOBAL_MEM_FENCE);
    }
    else {
        PUT_READY_TO_RECEIVE(i_ready_to_receive_flag);
        WAIT_INPUT_DATA(left_wrote_to_me_flag, ready_to_recv_sync_count);
        barrier(CLK_LOCAL_MEM_FENCE);

        if (my_rank != last_rank) {
            WAIT_SIGNAL_TO_SEND(right_ready_to_recv_flag, can_send_sync_count);
            barrier(CLK_LOCAL_MEM_FENCE);
            for (size_t i = 0; i < segment_count; i++) {
                right_buffer[work_group_size * i + work_item_id] =
                    buffer[work_group_size * i + work_item_id];
                // output_buffer[work_group_size * i + work_item_id];
            }
            barrier(CLK_GLOBAL_MEM_FENCE);
            I_SENT(i_send_to_right_flag);
        }
    }

#ifdef KERNEL_DEBUG
    printf("kernel %zu.%d completed\n", my_rank, work_item_id);
#endif
}

__kernel void bcast_execution_char(size_t my_rank,
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

__kernel void bcast_execution_int(size_t my_rank,
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
__kernel void bcast_execution_bfp16(size_t my_rank,
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

__kernel void bcast_execution_double(size_t my_rank,
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

__kernel void bcast_execution_int64_t(size_t my_rank,
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

__kernel void bcast_execution_uint64_t(size_t my_rank,
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
// numa
__kernel void bcast_execution_numa_char(size_t my_rank,
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

__kernel void bcast_execution_numa_int(size_t my_rank,
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

__kernel void bcast_execution_numa_bfp16(size_t my_rank,
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

__kernel void bcast_execution_numa_float(size_t my_rank,
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

__kernel void bcast_execution_numa_double(size_t my_rank,
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

__kernel void bcast_execution_numa_int64_t(size_t my_rank,
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

__kernel void bcast_execution_numa_uint64_t(size_t my_rank,
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
