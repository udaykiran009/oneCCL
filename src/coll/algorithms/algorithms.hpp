#pragma once

#include "sched/master_sched.hpp"
#include "sched/sched.hpp"
#include "internal_types.hpp"

#include <map>
#include <type_traits>

#define CCL_UNDEFINED_ALGO_ID (-1)

ccl::status ccl_coll_build_naive_bcast(ccl_sched* sched,
                                       ccl_buffer buf,
                                       size_t count,
                                       const ccl_datatype& dtype,
                                       int root,
                                       ccl_comm* comm);

ccl::status ccl_coll_build_scatter_ring_allgather_bcast(ccl_sched* sched,
                                                        ccl_buffer buf,
                                                        size_t count,
                                                        const ccl_datatype& dtype,
                                                        int root,
                                                        ccl_comm* comm);

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
ccl::status ccl_coll_build_topo_bcast(ccl_sched* sched,
                                      ccl_buffer buf,
                                      size_t count,
                                      const ccl_datatype& dtype,
                                      int root,
                                      ccl_comm* comm);
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

ccl::status ccl_coll_build_dissemination_barrier(ccl_sched* sched, ccl_comm* comm);

ccl::status ccl_coll_build_rabenseifner_reduce(ccl_sched* sched,
                                               ccl_buffer send_buf,
                                               ccl_buffer recv_buf,
                                               size_t count,
                                               const ccl_datatype& dtype,
                                               ccl::reduction reduction,
                                               int root,
                                               ccl_comm* comm);

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
ccl::status ccl_coll_build_gpu_reduce(ccl_sched* sched,
                                      ccl_buffer send_buf,
                                      ccl_buffer recv_buf,
                                      size_t count,
                                      const ccl_datatype& dtype,
                                      ccl::reduction reduction,
                                      int root,
                                      ccl_comm* comm);
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

ccl::status ccl_coll_build_rabenseifner_allreduce(ccl_sched* sched,
                                                  ccl_buffer send_buf,
                                                  ccl_buffer recv_buf,
                                                  size_t count,
                                                  const ccl_datatype& dtype,
                                                  ccl::reduction reduction,
                                                  ccl_comm* comm);

ccl::status ccl_coll_build_binomial_reduce(ccl_sched* sched,
                                           ccl_buffer send_buf,
                                           ccl_buffer recv_buf,
                                           size_t count,
                                           const ccl_datatype& dtype,
                                           ccl::reduction reduction,
                                           int root,
                                           ccl_comm* comm);

ccl::status ccl_coll_build_ring_allreduce(ccl_sched* sched,
                                          ccl_buffer send_buf,
                                          ccl_buffer recv_buf,
                                          size_t count,
                                          const ccl_datatype& dtype,
                                          ccl::reduction reduction,
                                          ccl_comm* comm);

ccl::status ccl_coll_build_ring_rma_allreduce(ccl_sched* sched,
                                              ccl_buffer send_buf,
                                              ccl_buffer recv_buf,
                                              size_t count,
                                              const ccl_datatype& dtype,
                                              ccl::reduction reduction,
                                              ccl_comm* comm);

ccl::status ccl_coll_build_recursive_doubling_allreduce(ccl_sched* sched,
                                                        ccl_buffer send_buf,
                                                        ccl_buffer recv_buf,
                                                        size_t count,
                                                        const ccl_datatype& dtype,
                                                        ccl::reduction reduction,
                                                        ccl_comm* comm);

ccl::status ccl_coll_build_nreduce_allreduce(ccl_sched* sched,
                                             ccl_buffer send_buf,
                                             ccl_buffer recv_buf,
                                             size_t count,
                                             const ccl_datatype& dtype,
                                             ccl::reduction reduction,
                                             ccl_comm* comm);

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
ccl::status ccl_coll_build_topo_allreduce(ccl_sched* sched,
                                          ccl_buffer send_buf,
                                          ccl_buffer recv_buf,
                                          size_t count,
                                          const ccl_datatype& dtype,
                                          ccl::reduction reduction,
                                          ccl_comm* comm);

#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

ccl::status ccl_coll_build_naive_allgatherv(ccl_sched* sched,
                                            ccl_buffer send_buf,
                                            size_t send_count,
                                            ccl_buffer recv_buf,
                                            const size_t* recv_counts,
                                            const ccl_datatype& dtype,
                                            ccl_comm* comm);

class ccl_double_tree;
ccl::status ccl_coll_build_double_tree_op(ccl_sched* sched,
                                          ccl_coll_type coll_type,
                                          ccl_buffer send_buf,
                                          ccl_buffer recv_buf,
                                          size_t count,
                                          const ccl_datatype& dtype,
                                          ccl::reduction reduction,
                                          const ccl_double_tree& dtree,
                                          ccl_comm* comm);

ccl::status ccl_coll_build_ring_reduce_scatter(ccl_sched* sched,
                                               ccl_buffer send_buf,
                                               ccl_buffer recv_buf,
                                               size_t send_count,
                                               const ccl_datatype& dtype,
                                               ccl::reduction reduction,
                                               ccl_comm* comm);

ccl::status ccl_coll_build_ring_reduce_scatter_block(ccl_sched* sched,
                                                     ccl_buffer send_buf,
                                                     ccl_buffer recv_buf,
                                                     size_t recv_count,
                                                     const ccl_datatype& dtype,
                                                     ccl::reduction reduction,
                                                     ccl_comm* comm);

ccl::status ccl_coll_build_ring_allgatherv(ccl_sched* sched,
                                           ccl_buffer send_buf,
                                           size_t send_count,
                                           ccl_buffer recv_buf,
                                           const size_t* recv_counts,
                                           const ccl_datatype& dtype,
                                           ccl_comm* comm);

ccl::status ccl_coll_build_flat_allgatherv(ccl_master_sched* main_sched,
                                           std::vector<ccl_sched*>& scheds,
                                           const ccl_coll_param& coll_param);

ccl::status ccl_coll_build_multi_bcast_allgatherv(ccl_master_sched* main_sched,
                                                  std::vector<ccl_sched*>& scheds,
                                                  const ccl_coll_param& coll_param,
                                                  size_t data_partition_count);

ccl::status ccl_coll_build_topo_allgatherv(ccl_sched* sched,
                                           ccl_buffer send_buf,
                                           size_t send_count,
                                           ccl_buffer recv_buf,
                                           const size_t* recv_counts,
                                           const ccl_datatype& dtype,
                                           ccl_comm* comm);

ccl::status ccl_coll_build_naive_alltoallv(ccl_master_sched* main_sched,
                                           std::vector<ccl_sched*>& scheds,
                                           const ccl_coll_param& coll_param);

ccl::status ccl_coll_build_scatter_alltoallv(ccl_master_sched* main_sched,
                                             std::vector<ccl_sched*>& scheds,
                                             const ccl_coll_param& coll_param);

/* direct algorithms - i.e. direct mapping on collective API from transport level */

ccl::status ccl_coll_build_direct_barrier(ccl_sched* sched, ccl_comm* comm);

ccl::status ccl_coll_build_direct_reduce(ccl_sched* sched,
                                         ccl_buffer send_buf,
                                         ccl_buffer recv_buf,
                                         size_t count,
                                         const ccl_datatype& dtype,
                                         ccl::reduction reduction,
                                         int root,
                                         ccl_comm* comm);

ccl::status ccl_coll_build_direct_allgatherv(ccl_sched* sched,
                                             ccl_buffer send_buf,
                                             size_t send_count,
                                             ccl_buffer recv_buf,
                                             const size_t* recv_counts,
                                             const ccl_datatype& dtype,
                                             ccl_comm* comm);

ccl::status ccl_coll_build_direct_allreduce(ccl_sched* sched,
                                            ccl_buffer send_buf,
                                            ccl_buffer recv_buf,
                                            size_t count,
                                            const ccl_datatype& dtype,
                                            ccl::reduction reduction,
                                            ccl_comm* comm);

ccl::status ccl_coll_build_direct_alltoall(ccl_sched* sched,
                                           ccl_buffer send_buf,
                                           ccl_buffer recv_buf,
                                           size_t count,
                                           const ccl_datatype& dtype,
                                           ccl_comm* comm);

ccl::status ccl_coll_build_direct_alltoallv(ccl_sched* sched,
                                            ccl_buffer send_buf,
                                            const size_t* send_counts,
                                            ccl_buffer recv_buf,
                                            const size_t* recv_counts,
                                            const ccl_datatype& dtype,
                                            ccl_comm* comm);

ccl::status ccl_coll_build_direct_bcast(ccl_sched* sched,
                                        ccl_buffer buf,
                                        size_t count,
                                        const ccl_datatype& dtype,
                                        int root,
                                        ccl_comm* comm);

ccl::status ccl_coll_build_direct_reduce_scatter(ccl_sched* sched,
                                                 ccl_buffer send_buf,
                                                 ccl_buffer recv_buf,
                                                 size_t send_count,
                                                 const ccl_datatype& dtype,
                                                 ccl::reduction reduction,
                                                 ccl_comm* comm);

ccl::status ccl_coll_build_topo_reduce_scatter(ccl_sched* sched,
                                               ccl_buffer send_buf,
                                               ccl_buffer recv_buf,
                                               size_t send_count,
                                               const ccl_datatype& dtype,
                                               ccl::reduction reduction,
                                               ccl_comm* comm);
