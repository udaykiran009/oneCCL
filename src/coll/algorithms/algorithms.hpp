#pragma once

#include "sched/sched.hpp"

#include <map>
#include <type_traits>

#define CCL_UNDEFINED_ALGO_ID (-1)

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
