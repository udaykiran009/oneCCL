#ifndef SHARED_H
#define SHARED_H

/* Defines values and functions shared on both host and device */

/* This is defined as a macro since the value is used by the kernel code at compile-time */
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

#endif /* SHARED_H */