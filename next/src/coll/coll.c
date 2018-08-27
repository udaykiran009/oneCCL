#include "coll.h"
#include "coll_algorithms.h"
#include "global.h"
#include "sched.h"
#include "utils.h"

mlsl_status_t mlsl_coll_build_barrier(mlsl_sched *sched)
{
    mlsl_status_t status;
    sched->coll_desc->ctype = mlsl_coll_barrier;
    MLSL_CALL(mlsl_coll_build_dissemination_barrier(sched));
    return status;
}

mlsl_status_t mlsl_coll_build_bcast(mlsl_sched *sched, void *buf, size_t count, mlsl_data_type_t dtype, size_t root)
{
    mlsl_status_t status;
    sched->coll_desc->ctype = mlsl_coll_bcast;
    MLSL_CALL(mlsl_coll_build_scatter_ring_allgather_bcast(sched, buf, count, dtype, root));
    return status;
}

mlsl_status_t mlsl_coll_build_reduce(mlsl_sched *sched, const void *send_buf, void *recv_buf, size_t count,
                                     mlsl_data_type_t dtype, mlsl_reduction_t reduction, size_t root)
{
    mlsl_status_t status;
    sched->coll_desc->ctype = mlsl_coll_reduce;

    if (count < global_data.comm->pof2)
        MLSL_CALL(mlsl_coll_build_binomial_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
    else
        MLSL_CALL(mlsl_coll_build_rabenseifner_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));

    return status;
}

mlsl_status_t mlsl_coll_build_allreduce(
    mlsl_sched *sched,
    const void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction)
{
    mlsl_status_t status;
    sched->coll_desc->ctype = mlsl_coll_allreduce;

    if (count < global_data.comm->pof2 /*|| count * mlsl_get_dtype_size(dtype) <= 4096*/)
        MLSL_CALL(mlsl_coll_build_recursive_doubling_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
    else
        MLSL_CALL(mlsl_coll_build_rabenseifner_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));

    return status;
}

mlsl_status_t mlsl_sched_bcast(
    void *buf,
    size_t count,
    mlsl_data_type_t dtype,
    size_t root,
    mlsl_sched_t *sched)
{
    mlsl_status_t status = mlsl_status_success;

    MLSL_CALL(mlsl_sched_create(sched));

    mlsl_coll_desc *desc = (*sched)->coll_desc;
    desc->ctype = mlsl_coll_bcast;
    desc->buf = buf;
    desc->count = count;
    desc->dtype = dtype;
    desc->root = root;
    desc->comm = global_data.comm;

    return status;
}

mlsl_status_t mlsl_sched_reduce(
    const void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    size_t root,
    mlsl_sched_t *sched)
{
    mlsl_status_t status = mlsl_status_success;

    MLSL_CALL(mlsl_sched_create(sched));

    mlsl_coll_desc *desc = (*sched)->coll_desc;
    desc->ctype = mlsl_coll_reduce;
    desc->send_buf = send_buf;
    desc->recv_buf = recv_buf;
    desc->count = count;
    desc->dtype = dtype;
    desc->reduction = reduction;
    desc->root = root;
    desc->comm = global_data.comm;

    return status;
}

mlsl_status_t mlsl_sched_allreduce(
    const void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    mlsl_sched_t *sched)
{
    mlsl_status_t status = mlsl_status_success;

    MLSL_CALL(mlsl_sched_create(sched));

    mlsl_coll_desc *desc = (*sched)->coll_desc;
    desc->ctype = mlsl_coll_allreduce;
    desc->send_buf = send_buf;
    desc->recv_buf = recv_buf;
    desc->count = count;
    desc->dtype = dtype;
    desc->reduction = reduction;
    desc->comm = global_data.comm;

    return status;
}

mlsl_status_t mlsl_sched_allgatherv(
    const void *send_buf,
    size_t send_count,
    void *recv_buf,
    size_t *recv_counts,
    mlsl_data_type_t dtype,
    mlsl_sched_t *sched)
{
    mlsl_status_t status = mlsl_status_success;

    MLSL_CALL(mlsl_sched_create(sched));

    mlsl_coll_desc *desc = (*sched)->coll_desc;
    desc->ctype = mlsl_coll_allgatherv;
    desc->send_buf = send_buf;
    desc->send_count = send_count;
    desc->recv_buf = recv_buf;
    desc->recv_counts = recv_counts;
    desc->dtype = dtype;
    desc->comm = global_data.comm;

    return status;
}

mlsl_status_t mlsl_sched_barrier(mlsl_sched_t *sched)
{
    mlsl_status_t status = mlsl_status_success;

    MLSL_CALL(mlsl_sched_create(sched));

    mlsl_coll_desc *desc = (*sched)->coll_desc;
    desc->ctype = mlsl_coll_barrier;
    desc->dtype = mlsl_dtype_char;
    desc->comm = global_data.comm;

    return status;
}

mlsl_status_t mlsl_bcast(
    void *buf,
    size_t count,
    mlsl_data_type_t dtype,
    size_t root,
    mlsl_request_t *req)
{
    mlsl_status_t status;
    mlsl_sched *sched;

    MLSL_CALL(mlsl_sched_bcast(buf, count, dtype, root, &sched));
    MLSL_CALL(mlsl_sched_commit_with_type(sched, mlsl_sched_non_persistent));
    MLSL_CALL(mlsl_sched_start(sched, req));

    return status;
}

mlsl_status_t mlsl_reduce(
    const void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    size_t root,
    mlsl_request_t *req)
{
    mlsl_status_t status;
    mlsl_sched *sched;

    MLSL_CALL(mlsl_sched_reduce(send_buf, recv_buf, count, dtype, reduction, root, &sched));
    MLSL_CALL(mlsl_sched_commit_with_type(sched, mlsl_sched_non_persistent));
    MLSL_CALL(mlsl_sched_start(sched, req));

    return status;
}

mlsl_status_t mlsl_allreduce(
    const void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    mlsl_request **req)
{
    mlsl_status_t status;
    mlsl_sched *sched;

    MLSL_CALL(mlsl_sched_allreduce(send_buf, recv_buf, count, dtype, reduction, &sched));
    MLSL_CALL(mlsl_sched_commit_with_type(sched, mlsl_sched_non_persistent));
    MLSL_CALL(mlsl_sched_start(sched, req));

    return status;
}

mlsl_status_t mlsl_allgatherv(
    const void *send_buf,
    size_t send_count,
    void *recv_buf,
    size_t *recv_counts,
    mlsl_data_type_t dtype,
    mlsl_request_t *req)
{
    mlsl_status_t status;
    mlsl_sched *sched;

    MLSL_CALL(mlsl_sched_allgatherv(send_buf, send_count, recv_buf, recv_counts, dtype, &sched));
    MLSL_CALL(mlsl_sched_commit_with_type(sched, mlsl_sched_non_persistent));
    MLSL_CALL(mlsl_sched_start(sched, req));

    return status;
}

mlsl_status_t mlsl_barrier()
{
    mlsl_status_t status;
    mlsl_sched *sched;
    mlsl_request *req;

    MLSL_CALL(mlsl_sched_barrier(&sched));
    MLSL_CALL(mlsl_sched_commit_with_type(sched, mlsl_sched_non_persistent));
    MLSL_CALL(mlsl_sched_start(sched, &req));
    MLSL_CALL(mlsl_wait(req));

    return status;
}
