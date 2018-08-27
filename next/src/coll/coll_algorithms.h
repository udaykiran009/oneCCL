#ifndef COLL_ALGORITHMS_H
#define COLL_ALGORITHMS_H

#include "global.h"
#include "sched.h"

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

#endif /* COLL_ALGORITHMS_H */
