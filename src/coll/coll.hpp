#pragma once

#include "common/comm/comm.hpp"
#include "common/datatype/datatype.hpp"

#define MLSL_INVALID_PROC_IDX (-1)

struct mlsl_sched;

enum mlsl_coll_type
{
    mlsl_coll_barrier            = 0,
    mlsl_coll_bcast              = 1,
    mlsl_coll_reduce             = 2,
    mlsl_coll_allreduce          = 3,
    mlsl_coll_allgatherv         = 4,
    mlsl_coll_service_temporal   = 5,
    mlsl_coll_service_persistent = 6,
    mlsl_coll_none               = 7,
    mlsl_coll_sparse_allreduce   = 8
};

extern mlsl_coll_attr_t *default_coll_attr;

mlsl_status_t mlsl_coll_create_attr(mlsl_coll_attr_t **coll_attr);

mlsl_status_t mlsl_coll_free_attr(mlsl_coll_attr_t *coll_attr);

mlsl_status_t mlsl_coll_build_barrier(mlsl_sched *sched);

mlsl_status_t mlsl_coll_build_bcast(mlsl_sched *sched, void *buf, size_t count, mlsl_datatype_internal_t dtype, size_t root);

mlsl_status_t mlsl_coll_build_reduce(mlsl_sched *sched, const void *send_buf, void *recv_buf, size_t count,
                                     mlsl_datatype_internal_t dtype, mlsl_reduction_t reduction, size_t root);

mlsl_status_t mlsl_coll_build_allreduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                        size_t count, mlsl_datatype_internal_t dtype, mlsl_reduction_t reduction);

mlsl_status_t mlsl_coll_build_sparse_allreduce(mlsl_sched *sched, const void *send_ind_buf, size_t send_ind_count,
                                               const void *send_val_buf, size_t send_val_count, void **recv_ind_buf,
                                               size_t *recv_ind_count, void **recv_val_buf, size_t *recv_val_count,
                                               mlsl_datatype_internal_t index_dtype, mlsl_datatype_internal_t value_dtype,
                                               mlsl_reduction_t reduction);

const char *mlsl_coll_type_to_str(mlsl_coll_type type);
