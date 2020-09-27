#ifdef HOST_CTX
#define __global
using namespace ccl;
#else
typedef ushort bf16;
#endif

typedef struct __attribute__((packed)) a2a_gpu_comm_data_char {
    __global char* recv_buf;
    __global int* ready_to_receive_flag;
    __global int* data_sent_flag;
} a2a_gpu_comm_data_char;

typedef struct __attribute__((packed)) a2a_gpu_comm_data_int {
    __global int* recv_buf;
    __global int* ready_to_receive_flag;
    __global int* data_sent_flag;
} a2a_gpu_comm_data_int;

typedef struct __attribute__((packed)) a2a_gpu_comm_data_float {
    __global float* recv_buf;
    __global int* ready_to_receive_flag;
    __global int* data_sent_flag;
} a2a_gpu_comm_data_float;

typedef struct __attribute__((packed)) a2a_gpu_comm_data_bf16 {
    __global bf16* recv_buf;
    __global int* ready_to_receive_flag;
    __global int* data_sent_flag;
} a2a_gpu_comm_data_bf16;

typedef struct __attribute__((packed)) a2a_gpu_comm_data_double {
    __global double* recv_buf;
    __global int* ready_to_receive_flag;
    __global int* data_sent_flag;
} a2a_gpu_comm_data_double;

typedef struct __attribute__((packed)) a2a_gpu_comm_data_long {
    __global long* recv_buf;
    __global int* ready_to_receive_flag;
    __global int* data_sent_flag;
} a2a_gpu_comm_data_long;

typedef struct __attribute__((packed)) a2a_gpu_comm_data_ulong {
    __global ulong* recv_buf;
    __global int* ready_to_receive_flag;
    __global int* data_sent_flag;
} a2a_gpu_comm_data_ulong;
