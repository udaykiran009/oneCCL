#pragma once

#include "sched/sched.hpp"

#include <map>
#include <type_traits>

#define CCL_UNDEFINED_ALGO_ID (-1)

enum ccl_coll_allgatherv_algo
{
    ccl_coll_allgatherv_direct,
    ccl_coll_allgatherv_naive,
    ccl_coll_allgatherv_flat,
    ccl_coll_allgatherv_multi_bcast,

    ccl_coll_allgatherv_last_value
};

enum ccl_coll_allreduce_algo
{
    ccl_coll_allreduce_direct,
    ccl_coll_allreduce_rabenseifner,
    ccl_coll_allreduce_starlike,
    ccl_coll_allreduce_ring,
    ccl_coll_allreduce_ring_rma,
    ccl_coll_allreduce_double_tree,
    ccl_coll_allreduce_recursive_doubling,

    ccl_coll_allreduce_last_value
};

enum ccl_coll_barrier_algo
{
    ccl_coll_barrier_direct,
    ccl_coll_barrier_ring,

    ccl_coll_barrier_last_value
};

enum ccl_coll_bcast_algo
{
    ccl_coll_bcast_direct,
    ccl_coll_bcast_ring,
    ccl_coll_bcast_double_tree,
    ccl_coll_bcast_naive,

    ccl_coll_bcast_last_value
};

enum ccl_coll_reduce_algo
{
    ccl_coll_reduce_direct,
    ccl_coll_reduce_rabenseifner,
    ccl_coll_reduce_tree,
    ccl_coll_reduce_double_tree,

    ccl_coll_reduce_last_value
};

enum ccl_coll_sparse_allreduce_algo
{
    ccl_coll_sparse_allreduce_basic,
    ccl_coll_sparse_allreduce_mask,

    ccl_coll_sparse_allreduce_last_value
};

ccl_status_t ccl_coll_build_naive_bcast(ccl_sched* sched,
                                        ccl_buffer buf,
                                        size_t count,
                                        ccl_datatype_internal_t dtype,
                                        size_t root);

ccl_status_t ccl_coll_build_scatter_ring_allgather_bcast(ccl_sched* sched,
                                                         ccl_buffer buf,
                                                         size_t count,
                                                         ccl_datatype_internal_t dtype,
                                                         size_t root);

ccl_status_t ccl_coll_build_dissemination_barrier(ccl_sched* sched);

ccl_status_t ccl_coll_build_rabenseifner_reduce(ccl_sched* sched,
                                                ccl_buffer send_buf,
                                                ccl_buffer recv_buf,
                                                size_t count,
                                                ccl_datatype_internal_t dtype,
                                                ccl_reduction_t reduction,
                                                size_t root);

ccl_status_t ccl_coll_build_rabenseifner_allreduce(ccl_sched* sched,
                                                   ccl_buffer send_buf,
                                                   ccl_buffer recv_buf,
                                                   size_t count,
                                                   ccl_datatype_internal_t dtype,
                                                   ccl_reduction_t op);

ccl_status_t ccl_coll_build_binomial_reduce(ccl_sched* sched,
                                            ccl_buffer send_buf,
                                            ccl_buffer recv_buf,
                                            size_t count,
                                            ccl_datatype_internal_t dtype,
                                            ccl_reduction_t reduction,
                                            size_t root);

ccl_status_t ccl_coll_build_ring_allreduce(ccl_sched* sched,
                                           ccl_buffer send_buf,
                                           ccl_buffer recv_buf,
                                           size_t count,
                                           ccl_datatype_internal_t dtype,
                                           ccl_reduction_t op);

ccl_status_t ccl_coll_build_ring_rma_allreduce(ccl_sched* sched,
                                               ccl_buffer send_buf,
                                               ccl_buffer recv_buf,
                                               size_t count,
                                               ccl_datatype_internal_t dtype,
                                               ccl_reduction_t op);

ccl_status_t ccl_coll_build_recursive_doubling_allreduce(ccl_sched* sched,
                                                         ccl_buffer send_buf,
                                                         ccl_buffer recv_buf,
                                                         size_t count,
                                                         ccl_datatype_internal_t dtype,
                                                         ccl_reduction_t op);

ccl_status_t ccl_coll_build_starlike_allreduce(ccl_sched* sched,
                                               ccl_buffer send_buf,
                                               ccl_buffer recv_buf,
                                               size_t count,
                                               ccl_datatype_internal_t dtype,
                                               ccl_reduction_t op);

ccl_status_t ccl_coll_build_naive_allgatherv(ccl_sched* sched,
                                             ccl_buffer send_buf,
                                             size_t send_count,
                                             ccl_buffer recv_buf,
                                             const size_t* recv_counts,
                                             ccl_datatype_internal_t dtype);

ccl_status_t ccl_coll_build_sparse_allreduce_basic(ccl_sched* sched,
                                                   ccl_buffer send_ind_buf, size_t send_ind_count,
                                                   ccl_buffer send_val_buf, size_t send_val_count,
                                                   ccl_buffer recv_ind_buf, size_t* recv_ind_count,
                                                   ccl_buffer recv_val_buf, size_t* recv_val_count,
                                                   ccl_datatype_internal_t index_dtype,
                                                   ccl_datatype_internal_t value_dtype,
                                                   ccl_reduction_t op);

class ccl_double_tree;
ccl_status_t ccl_coll_build_double_tree_op(ccl_sched* sched,
                                           ccl_coll_type coll_type,
                                           ccl_buffer send_buf,
                                           ccl_buffer recv_buf,
                                           size_t count,
                                           ccl_datatype_internal_t dtype,
                                           ccl_reduction_t op,
                                           const ccl_double_tree& dtree);

/* direct algorithms - i.e. direct mapping on collective API from transport level */

ccl_status_t ccl_coll_build_direct_barrier(ccl_sched *sched);

ccl_status_t ccl_coll_build_direct_reduce(ccl_sched *sched,
                                          ccl_buffer send_buf,
                                          ccl_buffer recv_buf,
                                          size_t count,
                                          ccl_datatype_internal_t dtype,
                                          ccl_reduction_t reduction,
                                          size_t root);

ccl_status_t ccl_coll_build_direct_allgatherv(ccl_sched* sched,
                                              ccl_buffer send_buf,
                                              size_t send_count,
                                              ccl_buffer recv_buf,
                                              const size_t* recv_counts,
                                              ccl_datatype_internal_t dtype);


ccl_status_t ccl_coll_build_direct_allreduce(ccl_sched *sched,
                                             ccl_buffer send_buf,
                                             ccl_buffer recv_buf,
                                             size_t count,
                                             ccl_datatype_internal_t dtype,
                                             ccl_reduction_t op);

ccl_status_t ccl_coll_build_direct_bcast(ccl_sched *sched,
                                         ccl_buffer buf,
                                         size_t count,
                                         ccl_datatype_internal_t dtype,
                                         size_t root);
