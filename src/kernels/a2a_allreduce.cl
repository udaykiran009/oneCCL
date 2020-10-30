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
                _desired += comm_size; \
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

__kernel void allreduce_execution_float32(int rank_id,
                                        int comm_size,
                                        size_t elems_count,
                                        const __global float* input_buffer,
                                        __global float* output_buffer,
                                        __global a2a_gpu_comm_data_float32* comm_matrix) {
    //Limitations:
    //1) WG count is equal to ranks

    size_t wg_id = get_group_id(0);
    size_t wg_size = get_local_size(0);
    size_t wg_count = get_num_groups(0);
    size_t local_thread_id = get_local_id(0);
    size_t g_thread_id = get_global_id(0);

    int ready_to_recv_sync_count = comm_size;
    int can_send_sync_count = comm_size;

    size_t segment_size = wg_size * wg_count; //elems_count / comm_size;

    //   ________________________________________________________________________
    //  |                      elems_count                                      |
    //  -------------------------------------------------------------------------
    //   ________________________________________________________________________
    //  | rank_0     |     rank_1 |             | rank_i |      | comm_size - 1 |
    //  -------------------------------------------------------------------------
    //  {segment_size}
    //
    //  !!! LIMITATION: WG count = comm size: WG <-> rank mapping
    //
    //  __________________rank_i_________________________________________________
    //  |  wg_0    |    wg_1  |          | wg_i|                   | wg_size - 1|
    //  -------------------------------------------------------------------------
    //  {  wg_count }

    //  Final
    //  __________________________________________________________________________________
    // |        r0_wg0         | r0_wg1 |..| r0_wgN |.....|ri_wg0|..|ri_wgN|......|rN_wgN|
    // -----------------------------------------------------------------------------------
    // { wg_size }       ...   {wg_size }  {wg_size }          =    wg_count
    // {        segment_size                        }          =    comm_size
    //

#ifdef KERNEL_DEBUG
    if (local_thread_id == 0) {
        printf("kernel %zu.%zu.%zu, wg_size: %zu, segment_size: %zu\n",
               rank_id,
               wg_id,
               local_thread_id,
               wg_size,
               segment_size);
    }
#endif

    for (size_t segment_start_idx = 0; segment_start_idx < elems_count;
         segment_start_idx += segment_size) {
#ifdef KERNEL_DEBUG
        if (local_thread_id == 0) {
            printf("kernel %zu.%zu.%zu, start from segment offset: %zu\n",
                   rank_id,
                   wg_id,
                   local_thread_id,
                   segment_start_idx);
            printf("kernel %zu.%zu.%zu, ready_to_receive_flag: %zu\n",
                   rank_id,
                   wg_id,
                   local_thread_id,
                   *comm_matrix[rank_id].ready_to_receive_flag);
            printf("kernel %zu.%zu.%zu, data_sent_flag: %zu\n",
                   rank_id,
                   wg_id,
                   local_thread_id,
                   *comm_matrix[rank_id].data_sent_flag);
        }
#endif
        PUT_READY_TO_RECEIVE(comm_matrix[rank_id].ready_to_receive_flag);

        //each WG waits rank ready to receive
        WAIT_SIGNAL_TO_SEND(comm_matrix[wg_id].ready_to_receive_flag, can_send_sync_count);
        barrier(CLK_LOCAL_MEM_FENCE);

        //1)reduce scatter
        //Send segment_size by wg_size to each rank determined by WG_idx <-> rank
        comm_matrix[wg_id].recv_buf[rank_id * wg_size + local_thread_id] =
            input_buffer[segment_start_idx + wg_size * wg_id + local_thread_id];
#ifdef KERNEL_DEBUG
        {
            printf("kernel %zu.%zu.%zu send: %e by offset: %zu\n",
                   rank_id,
                   wg_id,
                   local_thread_id,
                   input_buffer[segment_start_idx + wg_size * wg_id + local_thread_id],
                   segment_start_idx + wg_size * wg_id + local_thread_id);
        }
#endif
        barrier(CLK_LOCAL_MEM_FENCE);
        I_SENT(comm_matrix[wg_id].data_sent_flag);

        //Wait for WG sent
        WAIT_INPUT_DATA(comm_matrix[rank_id].data_sent_flag, ready_to_recv_sync_count);
        barrier(CLK_GLOBAL_MEM_FENCE); //wait all WGs

        //Local reduce of initial chunk
        //
        // WG_i:
        // _________________________________________________________________
        // |                              tmp_recv_buf                      |
        // ------------------------------------------------------------------
        //
        // _________________________________________________________________
        // |  segment_size |  segement_size  |...............| segment_size |
        // -----------------------------------------------------------------
        // |||             |||                               |||
        // |||             |||                               |||
        // ||| +wg_size--> |||    + wg_size * N-i --->       |||
        //G0_WI_0,1,2     G0_WI_0   |                       G0_WI_0[1,2...WI_COUNT]
        //  ||              ||                                ||
        // G1_WI_0,1,2     G1_WI_0                           G1_WI_0[1,2...WI_COUNT]
        //   |               |                                 |
        //  G2_WI_0,1,2,    G2_WI_0                           G2_WI_0[1,2...WI_COUNT]

        //   G0_WI_0 = Sum(tmp_recv_buf[0], tmp_recv_buf[1], ..., tmp_recv_buf[N])
        //   G1_WI_0 = Sum(tmp_recv_buf[0], tmp_recv_buf[1], ..., tmp_recv_buf[N])
        //
        //   <TODO> make thread-reduce-chunk != 1 to optimize cache
        //
        //   GN_WI_0 = Sum(tmp_recv_buf[0], tmp_recv_buf[1], ..., tmp_recv_buf[N])
        //
        //   G0 = G1 = G2
        //
        //  Each WG_i[0...N] contains reduced values from all ranks at index i[0...N]
        // in its registers
        //  _____________________________________________
        //  |WI_0_reduced | WI_1_reduced | Wi_2_reduced |
        //  ---------------------------------------------

        float thread_reduced_value = 0;
        for (size_t recv_buf_start_idx = 0; recv_buf_start_idx < segment_size;
             recv_buf_start_idx += wg_size) {
            thread_reduced_value +=
                comm_matrix[rank_id].recv_buf[recv_buf_start_idx + local_thread_id];
        }

        barrier(CLK_LOCAL_MEM_FENCE);
#ifdef KERNEL_DEBUG
        {
            printf("kernel %zu.%zu.%zu,thread_reduced_value: %e\n",
                   rank_id,
                   wg_id,
                   local_thread_id,
                   thread_reduced_value);
        }
#endif
        //now each thread in each wg contains reduced value in its register
        barrier(CLK_GLOBAL_MEM_FENCE);

        //scatter
        I_SENT(comm_matrix[wg_id].data_sent_flag);
        comm_matrix[wg_id].recv_buf[rank_id * wg_size + local_thread_id] = thread_reduced_value;

        barrier(CLK_GLOBAL_MEM_FENCE); //wait all WGs

        //Wait for WG sent
        WAIT_INPUT_DATA(comm_matrix[rank_id].data_sent_flag, ready_to_recv_sync_count);
        barrier(CLK_GLOBAL_MEM_FENCE); //wait all WGs

        //remember to itself from other devices
        output_buffer[segment_start_idx + wg_size * wg_id + local_thread_id] =
            comm_matrix[wg_id].recv_buf[wg_id * wg_size + local_thread_id];
        barrier(CLK_GLOBAL_MEM_FENCE);
    }
}
