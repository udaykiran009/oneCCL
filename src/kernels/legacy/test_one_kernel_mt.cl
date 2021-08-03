#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#pragma OPENCL EXTENSION cl_khr_depth_images : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable

__kernel void test_main(global int* kernel_id,
                        global float* this_send_buf,
                        global float* this_recv_buf,
                        global float* other_send_buf,
                        global float* other_recv_buf,
                        volatile global int* my_flag,
                        volatile global int* other_flag) {
    int thread_id = get_global_id(0);
    int data_offset = thread_id;
    //set my flag
    if (thread_id == 0) {
        printf("kernel id: %d - my flag before: %d\n", *kernel_id, *my_flag);
        atomic_inc(my_flag);
        printf("kernel wi: %d - my flag after: %d\n", *kernel_id, *my_flag);
    }

    //wait other flag
    if (thread_id == 0) {
        printf("kernel wi: %d - other flag before: %d\n", *kernel_id, *other_flag);
        while (1 != atomic_cmpxchg(other_flag, 1, 1)) {
            barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);
        };
        printf("kernel wi: %d - other flag after: %d\n", *kernel_id, *other_flag);
    }

    other_recv_buf[thread_id] = this_send_buf[thread_id];
    barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);
    this_recv_buf[thread_id] = other_send_buf[thread_id];
    barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);
    printf("this_recv_buf[%d]=%f\n", thread_id, this_recv_buf[thread_id]);
}
