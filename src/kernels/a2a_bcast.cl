#include "a2a_helpers.h"

#define THREAD_LOCAL_REDUCE_CHUNK 1

#define DEFINE_KERNEL(Name, T) \
    __kernel void bcast_execution_##Name(int my_rank, \
                                         int comm_size, \
                                         ulong elems_count, \
                                         const __global T *input_buffer, \
                                         __global a2a_gpu_comm_data_##Name *comm_matrix, \
                                         ulong root) { \
        size_t wg_id = get_group_id(0); \
        size_t wg_size = get_local_size(0); \
        size_t wg_count = get_num_groups(0); \
        size_t thread_id = get_local_id(0); \
        size_t g_thread_id = get_global_id(0); \
\
        int ready_to_recv_sync_count = 1; \
        int sync_distributed = 1 + comm_size; \
        int can_send_sync_count = comm_size; \
\
        size_t segment_size = wg_size * wg_count; \
\
        for ( \
            size_t segment_start_idx = 0; segment_start_idx < elems_count; \
            segment_start_idx += \
            segment_size) { /* Everybody should be ready to receive data, even root,                                                \
                because its data is not in its receive buffer yet.                                                  \
           Moreover, root is not going to receive all the data at this step, just specific parts of it.             \
           Thus, root splits data among all the communication participants.                                         \
           ready_to_receive_flag is incremented 4 times */ \
            PUT_READY_TO_RECEIVE(comm_matrix[my_rank].ready_to_receive_flag); \
\
            /* TODO consider prefetch void prefetch(const __global gentype *p, size_t num_gentypes) */ \
            DEBUG_BLOCK(if (thread_id == 0) { \
                printf("kernel %d.%zu.%zu, ready_to_receive_flag: %d\n", \
                       my_rank, \
                       wg_id, \
                       thread_id, \
                       *comm_matrix[my_rank].ready_to_receive_flag); \
                printf("kernel %d.%zu.%zu, data_sent_flag: %d\n", \
                       my_rank, \
                       wg_id, \
                       thread_id, \
                       *comm_matrix[my_rank].data_sent_flag); \
            }); \
\
            if (my_rank == root) { \
                DEBUG_BLOCK(if (thread_id == 0) { \
                    printf("kernel %d.%zu.%zu, start from segment offset: %zu\n", \
                           my_rank, \
                           wg_id, \
                           thread_id, \
                           segment_start_idx); \
                    printf("kernel %d.%zu.%zu send: %e by offset: %zu\n", \
                           my_rank, \
                           wg_id, \
                           thread_id, \
                           input_buffer[segment_start_idx + wg_size * wg_id + thread_id], \
                           segment_start_idx + wg_size * wg_id + thread_id); \
                }); \
\
                /* Every wg_id checks its corresponding rank for readiness to receive data */ \
                WAIT_SIGNAL_TO_SEND(comm_matrix[wg_id].ready_to_receive_flag, \
                                    can_send_sync_count); \
                barrier(CLK_LOCAL_MEM_FENCE); \
\
                comm_matrix[wg_id].recv_buf[segment_start_idx + wg_size * wg_id + thread_id] = \
                    input_buffer[segment_start_idx + wg_size * wg_id + thread_id]; \
                barrier(CLK_GLOBAL_MEM_FENCE); \
\
                /* Every wg_id sets this flag to inform its corresponding rank to check its receive buffer.         \
               Since wg_id = 0 will be used here, too, root will have to wait for input data later                  \
               data_sent_flag is incremented only once */ \
                I_SENT(comm_matrix[wg_id].data_sent_flag); \
                barrier(CLK_GLOBAL_MEM_FENCE); \
            } \
            else { \
                WAIT_INPUT_DATA(comm_matrix[my_rank].data_sent_flag, ready_to_recv_sync_count); \
                barrier(CLK_GLOBAL_MEM_FENCE); \
            } \
\
            /* Let the data exchange process begin. Everyone writes to all the others in parallel,                  \
           so that later every rank will have the whole message*/ \
            comm_matrix[wg_id].recv_buf[segment_start_idx + wg_size * my_rank + thread_id] = \
                comm_matrix[my_rank].recv_buf[segment_start_idx + wg_size * my_rank + thread_id]; \
            barrier( \
                CLK_GLOBAL_MEM_FENCE); /* Here every rank will report to all the others about the data sent.                                   \
           If every rank increments data_sent_flag of the others, then every rank should                            \
           run data_sent_flag incrementation comm_size times*/ \
            I_SENT(comm_matrix[wg_id].data_sent_flag); \
\
            WAIT_INPUT_DATA(comm_matrix[my_rank].data_sent_flag, sync_distributed); \
            barrier( \
                CLK_GLOBAL_MEM_FENCE); /* printf("[rank %zu] [segment %zu]: wg_id %zu, wg_size %zu, wg_count %zu, thread_id %zu,         \
                        g_thread_id %zu, ready_to_receive_flag %d (desired %d); can_send_sync_count %d\n",          \
                my_rank, segment_start_idx, wg_id, wg_size, wg_count, thread_id, g_thread_id,                 \
                *comm_matrix[wg_id].ready_to_receive_flag, ready_to_recv_sync_count, can_send_sync_count);*/ \
        } \
    }

DEFINE_KERNEL(int8, int8_t)
DEFINE_KERNEL(uint8, uint8_t)
DEFINE_KERNEL(int16, int16_t)
DEFINE_KERNEL(uint16, uint16_t)
DEFINE_KERNEL(int32, int32_t)
DEFINE_KERNEL(uint32, uint32_t)
DEFINE_KERNEL(int64, int64_t)
DEFINE_KERNEL(uint64, uint64_t)
DEFINE_KERNEL(float16, half)
DEFINE_KERNEL(float32, float)
DEFINE_KERNEL(float64, double)
DEFINE_KERNEL(bfloat16, ushort)
