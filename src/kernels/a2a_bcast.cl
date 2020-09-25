#include "a2a_helpers.h"

#ifdef KERNEL_DEBUG

#define LOG_INPUT_DATA_START(kern_id, wg_id) \
    printf("Kernel %zu,%zu wait income data \n", kern_id, wg_id)

#define LOG_INPUT_DATA_END(kern_id, wg_id) printf("Kernel %zu,%zu received data \n", kern_id, wg_id)

#define LOG_OUTGOING_DATA_START(kern_id, wg_id) \
    printf("Kernel %zu,%zu wait signal to send\n", kern_id, wg_id)

#define LOG_OUTGOING_DATA_END(kern_id, wg_id) \
    printf("Kernel %zu,%zu received signal to send\n", kern_id, wg_id)

#define LOG_SEND_PROGRESS(kern_id, wg_id, thread_id, flag, desired) \
    printf("Kernel %zu.%zu, %d, send %d/%d\n", kern_id, wg_id, thread_id, flag, desired)

#define LOG_BARRIER_PASSED(kern_id, thread_id) \
    printf("kernel %zu.%d barrier passed\n", kern_id, thread_id);

#define LOG_IN_BARRIER(kern_id, thread_id, flag, desired) \
    printf("kernel %zu.%d barrier %d/%d\n", kern_id, thread_id, flag, desired);

#else

#define LOG_INPUT_DATA_START(kern_id, wg_id)
#define LOG_INPUT_DATA_END(kern_id, wg_id)
#define LOG_OUTGOING_DATA_START(kern_id, wg_id)
#define LOG_OUTGOING_DATA_END(kern_id, wg_id)
#define LOG_SEND_PROGRESS(kern_id, wg_id, thread_id, flag, desired)

#define LOG_BARRIER_PASSED(kern_id, thread_id)

#define LOG_IN_BARRIER(kern_id, thread_id, flag, desired)

#endif

#define PUT_READY_TO_RECEIVE(_sync_flag) \
    if (local_thread_id == 0) { \
        atomic_inc(_sync_flag); \
    }

#define I_SENT(_sync_flag) \
    if (local_thread_id == 0) { \
        atomic_inc(_sync_flag); \
    }

#define WAIT_INPUT_DATA(_sync_flag, _desired) \
    if (local_thread_id == 0) { \
        LOG_INPUT_DATA_START(rank_id, wg_id); \
        while (1) { \
            int _old_value = atomic_cmpxchg(_sync_flag, _desired, _desired); \
            if (_old_value == _desired) { \
                LOG_INPUT_DATA_END(rank_id, wg_id); \
                _desired += 1 + comm_size; \
                break; \
            } \
        } \
    }

#define WAIT_SIGNAL_TO_SEND(_sync_flag, _desired) \
    if (local_thread_id == 0) { \
        LOG_OUTGOING_DATA_START(rank_id, wg_id); \
        while (_desired != atomic_cmpxchg(_sync_flag, _desired, _desired)) { \
        }; \
        LOG_OUTGOING_DATA_END(rank_id, wg_id); \
        _desired += comm_size; \
    }

#define THREAD_LOCAL_REDUCE_CHUNK 1

#ifdef KERNEL_DEBUG
#define DEBUG_BLOCK(block) block
#else
#define DEBUG_BLOCK(block)
#endif

#define DEFINE_KERNEL(Name, T)                                                                                      \
    __kernel void bcast_execution_##Name(size_t rank_id,                                                            \
                                      size_t comm_size,                                                             \
                                      size_t elems_count,                                                           \
                                      const __global T *input_buffer,                                               \
                                      __global a2a_gpu_comm_data_##T *comm_matrix,                                  \
                                      size_t root) {                                                                \
        size_t wg_id = get_group_id(0);                                                                             \
        size_t wg_size = get_local_size(0);                                                                         \
        size_t wg_count = get_num_groups(0);                                                                        \
        size_t local_thread_id = get_local_id(0);                                                                   \
        size_t g_thread_id = get_global_id(0);                                                                      \
                                                                                                                    \
        int ready_to_recv_sync_count = 1;                                                                           \
        int sync_distributed = 1 + comm_size;                                                                       \
        int can_send_sync_count = comm_size;                                                                        \
                                                                                                                    \
        size_t segment_size = wg_size * wg_count;                                                                   \
                                                                                                                    \
        for (size_t segment_start_idx = 0; segment_start_idx < elems_count;                                         \
             segment_start_idx += segment_size) {                                                                   \
            /* Everybody should be ready to receive data, even root,                                                \
                because its data is not in its receive buffer yet.                                                  \
           Moreover, root is not going to receive all the data at this step, just specific parts of it.             \
           Thus, root splits data among all the communication participants.                                         \
           ready_to_receive_flag is incremented 4 times */                                                          \
            PUT_READY_TO_RECEIVE(comm_matrix[rank_id].ready_to_receive_flag);                                       \
                                                                                                                    \
            /* TODO consider prefetch void prefetch(const __global gentype *p, size_t num_gentypes) */              \
            DEBUG_BLOCK(if (local_thread_id == 0) {                                                                 \
                printf("kernel %zu.%zu.%zu, ready_to_receive_flag: %zu\n",                                          \
                       rank_id,                                                                                     \
                       wg_id,                                                                                       \
                       local_thread_id,                                                                             \
                       *comm_matrix[rank_id].ready_to_receive_flag);                                                \
                printf("kernel %zu.%zu.%zu, data_sent_flag: %zu\n",                                                 \
                       rank_id,                                                                                     \
                       wg_id,                                                                                       \
                       local_thread_id,                                                                             \
                       *comm_matrix[rank_id].data_sent_flag);                                                       \
            });                                                                                                     \
                                                                                                                    \
            if (rank_id == root) {                                                                                  \
                DEBUG_BLOCK(if (local_thread_id == 0) {                                                             \
                    printf("kernel %zu.%zu.%zu, start from segment offset: %zu\n",                                  \
                           rank_id,                                                                                 \
                           wg_id,                                                                                   \
                           local_thread_id,                                                                         \
                           segment_start_idx);                                                                      \
                    printf("kernel %zu.%zu.%zu send: %e by offset: %zu\n",                                          \
                           rank_id,                                                                                 \
                           wg_id,                                                                                   \
                           local_thread_id,                                                                         \
                           input_buffer[segment_start_idx + wg_size * wg_id + local_thread_id],                     \
                           segment_start_idx + wg_size * wg_id + local_thread_id);                                  \
                });                                                                                                 \
                                                                                                                    \
                /* Every wg_id checks its corresponding rank for readiness to receive data */                       \
                WAIT_SIGNAL_TO_SEND(comm_matrix[wg_id].ready_to_receive_flag,                                       \
                                    can_send_sync_count);                                                           \
                barrier(CLK_LOCAL_MEM_FENCE);                                                                       \
                                                                                                                    \
                comm_matrix[wg_id]                                                                                  \
                    .recv_buf[segment_start_idx + wg_size * wg_id + local_thread_id] =                              \
                    input_buffer[segment_start_idx + wg_size * wg_id + local_thread_id];                            \
                barrier(CLK_GLOBAL_MEM_FENCE);                                                                      \
                                                                                                                    \
                /* Every wg_id sets this flag to inform its corresponding rank to check its receive buffer.         \
               Since wg_id = 0 will be used here, too, root will have to wait for input data later                  \
               data_sent_flag is incremented only once */                                                           \
                I_SENT(comm_matrix[wg_id].data_sent_flag);                                                          \
                barrier(CLK_GLOBAL_MEM_FENCE);                                                                      \
            }                                                                                                       \
            else {                                                                                                  \
                WAIT_INPUT_DATA(comm_matrix[rank_id].data_sent_flag, ready_to_recv_sync_count);                     \
                barrier(CLK_GLOBAL_MEM_FENCE);                                                                      \
            }                                                                                                       \
                                                                                                                    \
            /* Let the data exchange process begin. Everyone writes to all the others in parallel,                  \
           so that later every rank will have the whole message*/                                                   \
            comm_matrix[wg_id].recv_buf[segment_start_idx + wg_size * rank_id + local_thread_id] =                  \
                comm_matrix[rank_id]                                                                                \
                    .recv_buf[segment_start_idx + wg_size * rank_id + local_thread_id];                             \
            barrier(CLK_GLOBAL_MEM_FENCE);                                                                          \
            /* Here every rank will report to all the others about the data sent.                                   \
           If every rank increments data_sent_flag of the others, then every rank should                            \
           run data_sent_flag incrementation comm_size times*/                                                      \
            I_SENT(comm_matrix[wg_id].data_sent_flag);                                                              \
                                                                                                                    \
            WAIT_INPUT_DATA(comm_matrix[rank_id].data_sent_flag, sync_distributed);                                 \
            barrier(CLK_GLOBAL_MEM_FENCE);                                                                          \
            /* printf("[rank %zu] [segment %zu]: wg_id %zu, wg_size %zu, wg_count %zu, local_thread_id %zu,         \
                        g_thread_id %zu, ready_to_receive_flag %d (desired %d); can_send_sync_count %d\n",          \
                rank_id, segment_start_idx, wg_id, wg_size, wg_count, local_thread_id, g_thread_id,                 \
                *comm_matrix[wg_id].ready_to_receive_flag, ready_to_recv_sync_count, can_send_sync_count);*/        \
        }                                                                                                           \
}

DEFINE_KERNEL(int8_t, char)
DEFINE_KERNEL(uint8_t, uchar)

DEFINE_KERNEL(int16_t, short)
DEFINE_KERNEL(uint16_t, ushort)

DEFINE_KERNEL(int32_t, int)
DEFINE_KERNEL(uint32_t, uint)

DEFINE_KERNEL(int64_t, long)
DEFINE_KERNEL(uint64_t, ulong)

DEFINE_KERNEL(float32_t, float)
DEFINE_KERNEL(float64_t, double)
// TODO: implement support for missing types
//DEFINE_KERNEL(float16_t, half)

DEFINE_KERNEL(bf16, ushort)
