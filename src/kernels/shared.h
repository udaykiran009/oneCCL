#ifndef SHARED_H
#define SHARED_H

// Defines values and functions shared on both host and device
// Constants VEC_SIZe is defined as a macro since the value is used by the kernel code at compile-time

// Allgatherv

#define RING_ALLGATHERV_VEC_SIZE 1

// Allreduce
#define RING_ALLREDUCE_VEC_SIZE 1

static inline size_t ring_allreduce_get_segment_size(size_t elems_count, size_t comm_size) {
    elems_count /= RING_ALLREDUCE_VEC_SIZE;
    return (elems_count + comm_size - 1) / comm_size;
}

static inline size_t ring_allreduce_get_tmp_buffer_size(size_t elems_count, size_t comm_size) {
    // The algorithm uses at most 2 * segment_size elements of tmp_buffer in order to store
    // temporal data
    return 2 * ring_allreduce_get_segment_size(elems_count, comm_size);
}

// Bcast

#define RING_BCAST_VEC_SIZE 1

// Reduce

#define RING_REDUCE_VEC_SIZE 1

static inline size_t ring_reduce_tmp_buffer_size(size_t elems_count, size_t comm_size) {
    (void)comm_size;
    return elems_count;
}

// Reduce-scatter

#define RING_REDUCE_SCATTER_VEC_SIZE 1

static inline size_t ring_reduce_scatter_get_segment_size(size_t recv_count, size_t comm_size) {
    (void)
        comm_size; // C disallows unnamed parameters in function signature, so use a nammed one and simply suppress it
    // Our segment size siply equal to recv_count parameter with respect to vector size
    recv_count /= RING_REDUCE_SCATTER_VEC_SIZE;
    return recv_count;
}

static inline size_t ring_reduce_scatter_tmp_buffer_size(size_t elems_count, size_t comm_size) {
    // The algorithm uses at most 2 * segment_size elements of tmp_buffer in order to store
    // temporal data
    return 2 * ring_reduce_scatter_get_segment_size(elems_count, comm_size);
}

#endif /* SHARED_H */