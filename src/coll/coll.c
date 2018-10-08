#include "coll/coll.h"
#include "coll/coll_algorithms.h"
#include "sched/sched.h"
#include "common/global/global.h"
#include "common/utils/utils.h"

mlsl_coll_attr_t *default_coll_attr = NULL;

mlsl_status_t mlsl_coll_create_attr(mlsl_coll_attr_t **coll_attr)
{
    mlsl_coll_attr_t *attr = MLSL_CALLOC(sizeof(mlsl_coll_attr_t), "coll_attr");
    *coll_attr = attr;
    return mlsl_status_success;
}

mlsl_status_t mlsl_coll_free_attr(mlsl_coll_attr_t *coll_attr)
{
    MLSL_FREE(coll_attr);
    return mlsl_status_success;
}

const char *mlsl_coll_type_to_str(mlsl_coll_type type)
{
    switch (type) {
        case mlsl_coll_barrier:
            return "BARRIER";
        case mlsl_coll_bcast:
            return "BCAST";
        case mlsl_coll_reduce:
            return "REDUCE";
        case mlsl_coll_allreduce:
            return "ALLREDUCE";
        case mlsl_coll_allgatherv:
            return "ALLGATHERV";
        case mlsl_coll_custom:
            return "CUSTOM";
        default:
            MLSL_ASSERT_FMT(0, "unexpected coll_type %d", type);
            return "(out of range)";
    }
}

#define MLSL_COLL(coll_attr, fill_cache_key_expr, create_sched_expr)                \
  do {                                                                              \
    const struct mlsl_coll_attr *attr = ((uintptr_t)coll_attr != (uintptr_t)NULL) ? \
        coll_attr : global_data.default_coll_attr;                                  \
    mlsl_sched *sched = NULL;                                                       \
    mlsl_sched_cache_entry *entry = NULL;                                           \
    if (attr->to_cache)                                                             \
    {                                                                               \
        mlsl_sched_cache_key key;                                                   \
        memset(&key, 0, sizeof(mlsl_sched_cache_key));                              \
        fill_cache_key_expr;                                                        \
        if (attr->match_id)                                                         \
            strncpy(key.match_id, attr->match_id, MLSL_MATCH_ID_MAX_LEN - 1);       \
        mlsl_sched_cache_get_entry(global_data.sched_cache, &key, &entry);          \
        sched = entry->sched;                                                       \
    }                                                                               \
    if (!sched)                                                                     \
    {                                                                               \
        create_sched_expr;                                                          \
        MLSL_LOG(DEBUG, "didn't find sched, create new one %p, type %s",            \
                 sched, mlsl_coll_type_to_str(sched->coll_param.ctype));            \
        MLSL_CALL(mlsl_sched_set_coll_attr(sched, attr));                           \
        MLSL_CALL(mlsl_sched_commit(sched));                                        \
        if (entry) entry->sched = sched;                                            \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        MLSL_LOG(DEBUG, "found sched, reuse %p, type %s",                           \
                 sched, mlsl_coll_type_to_str(sched->coll_param.ctype));            \
        MLSL_CALL(mlsl_sched_set_coll_attr(sched, attr));                           \
    }                                                                               \
    MLSL_CALL(mlsl_sched_start(sched, req));                                        \
    if (attr->synchronous)                                                          \
    {                                                                               \
        mlsl_wait(*req);                                                            \
        *req = NULL;                                                                \
    }                                                                               \
  } while (0)

mlsl_status_t mlsl_coll_build_barrier(mlsl_sched *sched)
{
    mlsl_status_t status;
    sched->coll_param.ctype = mlsl_coll_barrier;
    MLSL_CALL(mlsl_coll_build_dissemination_barrier(sched));
    return status;
}

mlsl_status_t mlsl_coll_build_bcast(mlsl_sched *sched, void *buf, size_t count, mlsl_data_type_t dtype, size_t root)
{
    mlsl_status_t status;
    sched->coll_param.ctype = mlsl_coll_bcast;
    MLSL_CALL(mlsl_coll_build_scatter_ring_allgather_bcast(sched, buf, count, dtype, root));
    return status;
}

mlsl_status_t mlsl_coll_build_reduce(mlsl_sched *sched, const void *send_buf, void *recv_buf, size_t count,
                                     mlsl_data_type_t dtype, mlsl_reduction_t reduction, size_t root)
{
    mlsl_status_t status;
    sched->coll_param.ctype = mlsl_coll_reduce;

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
    sched->coll_param.ctype = mlsl_coll_allreduce;

    if ((count < global_data.comm->pof2) || (count * mlsl_get_dtype_size(dtype) <= 8192))
    {
        MLSL_CALL(mlsl_coll_build_recursive_doubling_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
    }
    else
    {
        switch (env_data.allreduce_algo)
        {
            case mlsl_allreduce_algo_rabenseifner:
                MLSL_CALL(mlsl_coll_build_rabenseifner_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case mlsl_allreduce_algo_starlike:
                MLSL_CALL(mlsl_coll_build_starlike_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            default:
                MLSL_ASSERT_FMT(0, "unexpected allreduce_algo %d", env_data.allreduce_algo);
                return mlsl_status_invalid_arguments;
        }
    }

    return status;
}

mlsl_status_t mlsl_bcast(
    void *buf,
    size_t count,
    mlsl_data_type_t dtype,
    size_t root,
    const struct mlsl_coll_attr *attributes,
    mlsl_request **req)
{
    mlsl_status_t status;
    MLSL_COLL(attributes,
        {
            key.ctype = mlsl_coll_bcast;
            key.buf1 = buf;
            key.count1 = count;
            key.dtype = dtype;
            key.root = root;
        },
        {
            MLSL_CALL(mlsl_sched_bcast(buf, count, dtype, root, &sched));
        });
    return status;
}

mlsl_status_t mlsl_reduce(
    const void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    size_t root,
    const struct mlsl_coll_attr *attributes,
    mlsl_request **req)
{
    mlsl_status_t status;
    MLSL_COLL(attributes,
        {
            key.ctype = mlsl_coll_reduce;
            key.buf1 = (void *)send_buf;
            key.buf2 = recv_buf;
            key.count1 = count;
            key.dtype = dtype;
            key.reduction = reduction;
            key.root = root;
        },
        {
            MLSL_CALL(mlsl_sched_reduce(send_buf, recv_buf, count, dtype, reduction, root, &sched));
        });
    return status;
}

mlsl_status_t mlsl_allreduce(
    const void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    const struct mlsl_coll_attr *attributes,
    mlsl_request **req)
{
    mlsl_status_t status;
    MLSL_COLL(attributes,
        {
            key.ctype = mlsl_coll_allreduce;
            key.buf1 = (void *)send_buf;
            key.buf2 = recv_buf;
            key.count1 = count;
            key.dtype = dtype;
            key.reduction = reduction;
        },
        {
            MLSL_CALL(mlsl_sched_allreduce(send_buf, recv_buf, count, dtype, reduction, &sched));
        });
    return status;
}

mlsl_status_t mlsl_allgatherv(
    const void *send_buf,
    size_t send_count,
    void *recv_buf,
    size_t *recv_counts,
    mlsl_data_type_t dtype,
    const struct mlsl_coll_attr *attributes,
    mlsl_request **req)
{
    mlsl_status_t status;
    MLSL_COLL(attributes,
        {
            key.ctype = mlsl_coll_allgatherv;
            key.buf1 = (void *)send_buf;
            key.buf2 = recv_buf;
            key.count1 = send_count;
            key.dtype = dtype;
        },
        {
            MLSL_CALL(mlsl_sched_allgatherv(send_buf, send_count, recv_buf, recv_counts, dtype, &sched));
        });
    return status;
}

mlsl_status_t mlsl_barrier()
{
    mlsl_status_t status;
    mlsl_request *barrier_req;
    mlsl_request **req = &barrier_req;
    struct mlsl_coll_attr attributes;
    memset(&attributes, 0, sizeof(struct mlsl_coll_attr));
    attributes.synchronous = 1;
    MLSL_COLL((&attributes),
        {
            key.ctype = mlsl_coll_barrier;
        },
        {
            MLSL_CALL(mlsl_sched_barrier(&sched));
        });
    return status;
}
