#pragma once

#include "common/global/global.hpp"
#include "sched/sched.hpp"

mlsl_status_t mlsl_coll_build_dissemination_barrier(mlsl_sched *sched);

mlsl_status_t mlsl_coll_build_scatter_ring_allgather_bcast(mlsl_sched *sched, void *buf,
                                                           size_t count, mlsl_data_type_t dtype, size_t root);

mlsl_status_t mlsl_coll_build_rabenseifner_reduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                                  size_t count, mlsl_data_type_t dtype, mlsl_reduction_t reduction, size_t root);

mlsl_status_t mlsl_coll_build_binomial_reduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                              size_t count, mlsl_data_type_t dtype, mlsl_reduction_t reduction, size_t root);

mlsl_status_t mlsl_coll_build_rabenseifner_allreduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                                     size_t count, mlsl_data_type_t dtype, mlsl_reduction_t op);

mlsl_status_t mlsl_coll_build_recursive_doubling_allreduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                                           size_t count, mlsl_data_type_t dtype, mlsl_reduction_t op);

mlsl_status_t mlsl_coll_build_starlike_allreduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                                 size_t count, mlsl_data_type_t dtype, mlsl_reduction_t op);

mlsl_status_t mlsl_coll_build_naive_allgatherv(mlsl_sched* sched, const void* send_buf, size_t send_count,
                                               void* recv_buf, size_t* recv_counts, mlsl_data_type_t dtype);
