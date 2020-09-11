#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable

#include "event_declaration.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024024
#endif

#ifndef CHUNK_SIZE
#define CHUNK_SIZE 16
#endif

__kernel void numa_poll(const __global float* input_buffer,
                        __global float* output_buffer,

                        __global shared_event_float* events) {
    int wg_size = get_global_size(0);
    int wi_id = get_global_id(0);

    __local float local_buffer[1024];

    printf("events: counter=%d\n", *((*events).produced_bytes));
    size_t i = 0;
    size_t incr = 0;
    for (; i < BUFFER_SIZE; i += wg_size) {
        if (wi_id + i < BUFFER_SIZE) {
            local_buffer[wi_id] = input_buffer[i + wi_id] * (wi_id % wg_size);
            //barrier(CLK_LOCAL_MEM_FENCE);

            output_buffer[i + wi_id] = local_buffer[wi_id];
            events->mem_chunk[i + wi_id] = local_buffer[wi_id];

            atomic_add(events->produced_bytes, 1);
        }
        barrier(CLK_GLOBAL_MEM_FENCE);
    }

    printf("finished\n");
}
