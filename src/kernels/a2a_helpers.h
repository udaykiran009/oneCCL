#ifdef HOST_CTX
    #define __global
#endif


typedef struct __attribute__ ((packed)) a2a_gpu_comm_data_float
{
    __global float* recv_buf;
    __global int* ready_to_receive_flag;
    __global int* data_sent_flag;
} a2a_gpu_comm_data_float;
