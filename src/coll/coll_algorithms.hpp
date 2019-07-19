#pragma once

#include "common/global/global.hpp"
#include "sched/sched.hpp"

class double_tree;

iccl_status_t iccl_coll_build_scatter_ring_allgather_bcast(iccl_sched* sched,
                                                           iccl_buffer buf,
                                                           size_t count,
                                                           iccl_datatype_internal_t dtype,
                                                           size_t root);

iccl_status_t iccl_coll_build_dissemination_barrier(iccl_sched* sched);

iccl_status_t iccl_coll_build_rabenseifner_reduce(iccl_sched* sched,
                                                  iccl_buffer send_buf,
                                                  iccl_buffer recv_buf,
                                                  size_t count,
                                                  iccl_datatype_internal_t dtype,
                                                  iccl_reduction_t reduction,
                                                  size_t root);

iccl_status_t iccl_coll_build_rabenseifner_allreduce(iccl_sched* sched,
                                                     iccl_buffer send_buf,
                                                     iccl_buffer recv_buf,
                                                     size_t count,
                                                     iccl_datatype_internal_t dtype,
                                                     iccl_reduction_t op);

iccl_status_t iccl_coll_build_binomial_reduce(iccl_sched* sched,
                                              iccl_buffer send_buf,
                                              iccl_buffer recv_buf,
                                              size_t count,
                                              iccl_datatype_internal_t dtype,
                                              iccl_reduction_t reduction,
                                              size_t root);

iccl_status_t iccl_coll_build_ring_allreduce(iccl_sched* sched,
                                             iccl_buffer send_buf,
                                             iccl_buffer recv_buf,
                                             size_t count,
                                             iccl_datatype_internal_t dtype,
                                             iccl_reduction_t op);

iccl_status_t iccl_coll_build_ring_rma_allreduce(iccl_sched* sched,
                                                 iccl_buffer send_buf,
                                                 iccl_buffer recv_buf,
                                                 size_t count,
                                                 iccl_datatype_internal_t dtype,
                                                 iccl_reduction_t op);

iccl_status_t iccl_coll_build_recursive_doubling_allreduce(iccl_sched* sched,
                                                           iccl_buffer send_buf,
                                                           iccl_buffer recv_buf,
                                                           size_t count,
                                                           iccl_datatype_internal_t dtype,
                                                           iccl_reduction_t op);

iccl_status_t iccl_coll_build_starlike_allreduce(iccl_sched* sched,
                                                 iccl_buffer send_buf,
                                                 iccl_buffer recv_buf,
                                                 size_t count,
                                                 iccl_datatype_internal_t dtype,
                                                 iccl_reduction_t op);

iccl_status_t iccl_coll_build_naive_allgatherv(iccl_sched* sched,
                                               iccl_buffer send_buf,
                                               size_t send_count,
                                               iccl_buffer recv_buf,
                                               size_t* recv_counts,
                                               iccl_datatype_internal_t dtype);

iccl_status_t iccl_coll_build_sparse_allreduce_basic(iccl_sched* sched,
                                                     iccl_buffer send_ind_buf, size_t send_ind_count,
                                                     iccl_buffer send_val_buf, size_t send_val_count,
                                                     iccl_buffer recv_ind_buf, size_t* recv_ind_count,
                                                     iccl_buffer recv_val_buf, size_t* recv_val_count,
                                                     iccl_datatype_internal_t index_dtype,
                                                     iccl_datatype_internal_t value_dtype,
                                                     iccl_reduction_t op);

iccl_status_t iccl_coll_build_double_tree_op(iccl_sched* sched,
                                             iccl_coll_type coll_type,
                                             iccl_buffer send_buf,
                                             iccl_buffer recv_buf,
                                             size_t count,
                                             iccl_datatype_internal_t dtype,
                                             iccl_reduction_t op,
                                             const double_tree& dtree);

/* direct algorithms - i.e. direct mapping on collective API from transport level */

iccl_status_t iccl_coll_build_direct_barrier(iccl_sched *sched);

iccl_status_t iccl_coll_build_direct_reduce(iccl_sched *sched,
                                            iccl_buffer send_buf,
                                            iccl_buffer recv_buf,
                                            size_t count,
                                            iccl_datatype_internal_t dtype,
                                            iccl_reduction_t reduction,
                                            size_t root);

iccl_status_t iccl_coll_build_direct_allgatherv(iccl_sched* sched,
                                                iccl_buffer send_buf,
                                                size_t s_count,
                                                iccl_buffer recv_buf,
                                                size_t* r_counts,
                                                iccl_datatype_internal_t dtype);


iccl_status_t iccl_coll_build_direct_allreduce(iccl_sched *sched,
                                               iccl_buffer send_buf,
                                               iccl_buffer recv_buf,
                                               size_t count,
                                               iccl_datatype_internal_t dtype,
                                               iccl_reduction_t op);

iccl_status_t iccl_coll_build_direct_bcast(iccl_sched *sched,
                                           iccl_buffer buf,
                                           size_t count,
                                           iccl_datatype_internal_t dtype,
                                           size_t root);
