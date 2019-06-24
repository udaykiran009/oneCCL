#include "mlsl.hpp"
#include "coll/coll.hpp"
#include "coll/coll_algorithms.hpp"
#include "sched/sched_cache.hpp"
#include "common/request/request.hpp"
#include "common/utils/tree.hpp"
#include "out_of_order/ooo_match.hpp"
#include "fusion/fusion.hpp"

const char* mlsl_coll_type_to_str(mlsl_coll_type type)
{
    switch (type)
    {
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
        case mlsl_coll_sparse_allreduce:
            return "SPARSE_ALLREDUCE";
        case mlsl_coll_internal:
            return "INTERNAL";
        case mlsl_coll_none:
            return "NONE";
        default:
            MLSL_FATAL("unexpected coll_type ", type);
            return "UNKNOWN";
    }
}

static mlsl_request* mlsl_coll_create(const mlsl_coll_attr_t* attributes,
                                      mlsl_sched_key& key,
                                      mlsl_coll_param& coll_param)
{
    mlsl_sched* sched = nullptr;
    mlsl_comm* match_id_comm = nullptr;
    bool should_run = true;
    bool should_commit = false;
    bool was_fused = false;
    std::string match_id;
    mlsl_request* request = nullptr;
    const mlsl_coll_attr_t* attr = attributes ? attributes : global_data.default_coll_attr.get();

    MLSL_THROW_IF_NOT(coll_param.ctype == mlsl_coll_allreduce ||
                      !(attr->prologue_fn || attr->epilogue_fn || attr->reduction_fn),
                      "incorrect input");

    if (attr->match_id && env_data.out_of_order_support)
    {
        //copy user-defined match_id
        match_id.assign(attr->match_id);
        match_id_comm = global_data.ooo_manager->get_comm(match_id);
        if (!match_id_comm)
        {
            //user has provided match_id that has not been resolved yet.
            //schedule will be postponed until comm resolution
            should_run = false;
        }
        else
        {
            LOG_DEBUG("found comm id ", match_id_comm->id(), " for match_id ", match_id.c_str());
        }
    }

    if (attr->to_cache)
    {
        key.prologue_fn = attr->prologue_fn;
        key.epilogue_fn = attr->epilogue_fn;
        key.reduction_fn = attr->reduction_fn;
        key.priority = attr->priority;
        key.synchronous = attr->synchronous;
        //match_id contains empty string or user-defined match_id
        key.match_id = match_id;
        sched = global_data.sched_cache->find(key);
    }

    if (!sched)
    {
        sched = new mlsl_sched(coll_param);
        LOG_DEBUG("didn't find sched, create new one ", sched, ", type ",
                  mlsl_coll_type_to_str(sched->coll_param.ctype));

        if (attr->to_cache)
        {
            global_data.sched_cache->add(key, sched);
        }

        sched->set_coll_attr(attr, std::move(match_id));
        should_commit = true;
    }
    else
    {
        LOG_DEBUG("found sched, reuse ", sched, ", type ",
                  mlsl_coll_type_to_str(sched->coll_param.ctype));
    }
    if (match_id_comm)
    {
        sched->coll_param.comm = match_id_comm;
    }

    if (should_run)
    {
        if (env_data.enable_fusion)
        {
            was_fused = global_data.fusion_manager->add(sched);
            if (was_fused)
            {
                LOG_DEBUG("sched ", sched, ", ctype ", mlsl_coll_type_to_str(sched->coll_param.ctype), "will be fused");
                request = sched->req;
                return request;
            }
        }
    }

    MLSL_ASSERT(!was_fused);

    if (should_commit)
    {
        sched->commit(global_data.parallelizer.get());
    }

    if (should_run)
    {
        //normal schedule execution - either with user-defined or match_id specific communicator
        request = sched->start(global_data.executor.get());
        if (attr->synchronous)
        {
            mlsl_wait(request);
            request = nullptr;
        }
    }
    else
    {
        MLSL_ASSERT(!sched->coll_attr.match_id.empty(), "invalid match_id");

        request = sched->reset_request();
        LOG_INFO("sched ", sched, ", postponed for match_id resolution");
        global_data.ooo_manager->postpone(sched);
    }

    return request;
}

mlsl_status_t mlsl_coll_build_barrier(mlsl_sched* sched)
{
    mlsl_status_t status;
    sched->coll_param.ctype = mlsl_coll_barrier;

    switch (env_data.barrier_algo)
    {
        case mlsl_barrier_algo_ring:
            MLSL_CALL(mlsl_coll_build_dissemination_barrier(sched));
            break;
        case mlsl_barrier_algo_direct:
            MLSL_CALL(mlsl_coll_build_direct_barrier(sched));
            break;
        default:
            MLSL_FATAL("unexpected barrier_algo ",
                       mlsl_barrier_algo_to_str(env_data.barrier_algo));
            return mlsl_status_invalid_arguments;
    }

    return status;
}

mlsl_status_t mlsl_coll_build_bcast(mlsl_sched* sched,
                                    void* buf,
                                    size_t count,
                                    mlsl_datatype_internal_t dtype,
                                    size_t root)
{
    mlsl_status_t status;
    sched->coll_param.ctype = mlsl_coll_bcast;
    switch (env_data.bcast_algo)
    {
        case mlsl_bcast_algo_ring:
            MLSL_CALL(mlsl_coll_build_scatter_ring_allgather_bcast(sched, buf, count, dtype, root));
            break;
        case mlsl_bcast_algo_double_tree:
            MLSL_CALL(mlsl_coll_build_double_tree_op(sched, mlsl_coll_bcast, nullptr, buf, count, dtype,
                                                     mlsl_reduction_custom,
                                                     root == 0 ? sched->coll_param.comm->dtree() :
                                                     sched->coll_param.comm->dtree().copy_with_new_root(root)));
            break;
        case mlsl_bcast_algo_direct:
            MLSL_CALL(mlsl_coll_build_direct_bcast(sched, buf, count, dtype, root));
            break;
        default:
            MLSL_FATAL("unexpected bcast_algo ",
                       mlsl_bcast_algo_to_str(env_data.bcast_algo));
            return mlsl_status_invalid_arguments;
    }
    return status;
}

mlsl_status_t mlsl_coll_build_reduce(mlsl_sched* sched,
                                     const void* send_buf,
                                     void* recv_buf,
                                     size_t count,
                                     mlsl_datatype_internal_t dtype,
                                     mlsl_reduction_t reduction,
                                     size_t root)
{
    mlsl_status_t status;
    sched->coll_param.ctype = mlsl_coll_reduce;

    switch (env_data.reduce_algo)
    {
        case mlsl_reduce_algo_tree:
            if (count < sched->coll_param.comm->pof2())
                MLSL_CALL(mlsl_coll_build_binomial_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            else
                MLSL_CALL(mlsl_coll_build_rabenseifner_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            break;
        case mlsl_reduce_algo_double_tree:
            if (count < sched->coll_param.comm->pof2())
                MLSL_CALL(mlsl_coll_build_binomial_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            else
                MLSL_CALL(mlsl_coll_build_double_tree_op(sched, mlsl_coll_reduce, send_buf, recv_buf, count, dtype,
                                                         reduction,
                                                         root == 0 ? sched->coll_param.comm->dtree() :
                                                         sched->coll_param.comm->dtree().copy_with_new_root(root)));
            break;
        case mlsl_reduce_algo_direct:
            MLSL_CALL(mlsl_coll_build_direct_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            break;
        default:
            MLSL_FATAL("unexpected reduce_algo ",
                       mlsl_reduce_algo_to_str(env_data.reduce_algo));
            return mlsl_status_invalid_arguments;
    }

    return status;
}

mlsl_status_t mlsl_coll_build_allgatherv(
    mlsl_sched* sched,
    const void* send_buf,
    void* recv_buf,
    size_t s_count,
    size_t* r_counts,
    mlsl_datatype_internal_t dtype)
{
    mlsl_status_t status;
    sched->coll_param.ctype = mlsl_coll_allgatherv;
    switch (env_data.allgatherv_algo)
    {
        case mlsl_allgatherv_algo_naive:
            MLSL_CALL(mlsl_coll_build_naive_allgatherv(sched, send_buf, s_count, recv_buf, r_counts,
                                                       dtype));
            break;
        case mlsl_allgatherv_algo_direct:
            MLSL_CALL(mlsl_coll_build_direct_allgatherv(sched, send_buf, s_count, recv_buf, r_counts,
                                                       dtype));
            break;
        default:
            MLSL_FATAL("unexpected allgatherv_algo ",
                       mlsl_allgatherv_algo_to_str(env_data.allgatherv_algo));
            return mlsl_status_invalid_arguments;
    }
    return status;
}

mlsl_status_t mlsl_coll_build_allreduce(
    mlsl_sched* sched,
    const void* send_buf,
    void* recv_buf,
    size_t count,
    mlsl_datatype_internal_t dtype,
    mlsl_reduction_t reduction)
{
    mlsl_status_t status = mlsl_status_success;
    sched->coll_param.ctype = mlsl_coll_allreduce;

    if ((count < sched->coll_param.comm->pof2()) || (count * mlsl_datatype_get_size(dtype) <= 8192))
    {
        switch (env_data.allreduce_algo)
        {
            case mlsl_allreduce_algo_ring:
                MLSL_CALL(mlsl_coll_build_ring_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case mlsl_allreduce_algo_ring_rma:
                if (global_data.executor->is_rma_enabled)
                    MLSL_CALL(mlsl_coll_build_ring_rma_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                else
                    MLSL_FATAL("unexpected allreduce_algo ",
                               mlsl_allreduce_algo_to_str(env_data.allreduce_algo));
                break;
            case mlsl_allreduce_algo_direct:
                MLSL_CALL(mlsl_coll_build_direct_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            default:
                MLSL_CALL(
                    mlsl_coll_build_recursive_doubling_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
        }
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
            case mlsl_allreduce_algo_ring:
                MLSL_CALL(mlsl_coll_build_ring_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case mlsl_allreduce_algo_ring_rma:
                if (global_data.executor->is_rma_enabled)
                {
                    MLSL_CALL(mlsl_coll_build_ring_rma_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                    break;
                }
            case mlsl_allreduce_algo_double_tree:
                MLSL_CALL(mlsl_coll_build_double_tree_op(sched, mlsl_coll_allreduce, send_buf, recv_buf, count, dtype,
                                                         reduction,
                                                         sched->coll_param.comm->dtree()));
                break;
            case mlsl_allreduce_algo_direct:
                MLSL_CALL(mlsl_coll_build_direct_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            default:
                MLSL_FATAL("unexpected allreduce_algo ",
                           mlsl_allreduce_algo_to_str(env_data.allreduce_algo));
                return mlsl_status_invalid_arguments;
        }
    }

    return status;
}

mlsl_status_t mlsl_coll_build_sparse_allreduce(
    mlsl_sched* sched,
    const void* send_ind_buf,
    size_t send_ind_count,
    const void* send_val_buf,
    size_t send_val_count,
    void** recv_ind_buf,
    size_t* recv_ind_count,
    void** recv_val_buf,
    size_t* recv_val_count,
    mlsl_datatype_internal_t index_dtype,
    mlsl_datatype_internal_t value_dtype,
    mlsl_reduction_t reduction)
{
    mlsl_status_t status;
    sched->coll_param.ctype = mlsl_coll_sparse_allreduce;
    sched->coll_param.sparse_param.snd_val_buf = send_val_buf;
    sched->coll_param.sparse_param.snd_val_count = send_val_count;
    sched->coll_param.sparse_param.rcv_val_buf = recv_val_buf;
    sched->coll_param.sparse_param.rcv_val_count = recv_val_count;
    sched->coll_param.sparse_param.itype = index_dtype;

    switch (env_data.sparse_allreduce_algo)
    {
        case mlsl_sparse_allreduce_algo_basic:
            MLSL_CALL(mlsl_coll_build_sparse_allreduce_basic(sched, send_ind_buf, send_ind_count, send_val_buf,
                                                             send_val_count,
                                                             recv_ind_buf, recv_ind_count, recv_val_buf, recv_val_count,
                                                             index_dtype, value_dtype, reduction));
            break;
        default:
            MLSL_FATAL("unexpected sparse_allreduce_algo ", env_data.sparse_allreduce_algo);
            return mlsl_status_invalid_arguments;
    }

    return status;
}

mlsl_request* mlsl_sparse_allreduce_impl(const void* send_ind_buf,
                                         size_t send_ind_count,
                                         const void* send_val_buf,
                                         size_t send_val_count,
                                         void** recv_ind_buf,
                                         size_t* recv_ind_count,
                                         void** recv_val_buf,
                                         size_t* recv_val_count,
                                         mlsl_datatype_t index_dtype,
                                         mlsl_datatype_t dtype,
                                         mlsl_reduction_t reduction,
                                         const mlsl_coll_attr_t* attributes,
                                         mlsl_comm* communicator)
{
    mlsl_coll_param coll_param{};
    coll_param.ctype = mlsl_coll_sparse_allreduce;
    coll_param.send_buf = send_ind_buf;
    coll_param.send_count = send_ind_count;
    coll_param.sparse_param.snd_val_buf = send_val_buf;
    coll_param.sparse_param.snd_val_count = send_val_count;
    coll_param.sparse_param.rcv_ind_buf = recv_ind_buf;
    coll_param.recv_counts = recv_ind_count;
    coll_param.sparse_param.rcv_val_buf = recv_val_buf;
    coll_param.sparse_param.rcv_val_count = recv_val_count;
    coll_param.dtype = mlsl_datatype_get(dtype);
    coll_param.sparse_param.itype = mlsl_datatype_get(index_dtype);
    coll_param.reduction = reduction;
    coll_param.comm = communicator;

    mlsl_sched_key key{};
    key.ctype = mlsl_coll_sparse_allreduce;
    key.buf1 = (void*) send_ind_buf;
    key.count1 = send_ind_count;
    key.buf2 = (void*) send_val_buf;
    key.count2 = send_val_count;
    key.buf3 = recv_ind_buf;
    key.count3 = recv_ind_count;
    key.buf4 = recv_val_buf;
    key.count4 = recv_val_count;
    key.itype = index_dtype;
    key.dtype = dtype;
    key.reduction = reduction;
    key.comm = communicator;

    auto req = mlsl_coll_create(attributes, key, coll_param);
    LOG_DEBUG("coll ", mlsl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}

mlsl_request* mlsl_bcast_impl(void* buf,
                              size_t count,
                              mlsl_datatype_t dtype,
                              size_t root,
                              const mlsl_coll_attr_t* attributes,
                              mlsl_comm* communicator)
{
    mlsl_coll_param coll_param{};
    coll_param.ctype = mlsl_coll_bcast;
    coll_param.buf = buf;
    coll_param.count = count;
    coll_param.dtype = mlsl_datatype_get(dtype);
    coll_param.root = root;
    coll_param.comm = communicator;

    mlsl_sched_key key{};
    key.ctype = mlsl_coll_bcast;
    key.buf1 = buf;
    key.count1 = count;
    key.dtype = dtype;
    key.root = root;
    key.comm = communicator;

    auto req = mlsl_coll_create(attributes, key, coll_param);
    LOG_DEBUG("coll ", mlsl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}

mlsl_request* mlsl_reduce_impl(const void* send_buf,
                               void* recv_buf,
                               size_t count,
                               mlsl_datatype_t dtype,
                               mlsl_reduction_t reduction,
                               size_t root,
                               const mlsl_coll_attr_t* attributes,
                               mlsl_comm* communicator)
{
    mlsl_coll_param coll_param{};
    coll_param.ctype = mlsl_coll_reduce;
    coll_param.send_buf = send_buf;
    coll_param.recv_buf = recv_buf;
    coll_param.count = count;
    coll_param.dtype = mlsl_datatype_get(dtype);
    coll_param.reduction = reduction;
    coll_param.root = root;
    coll_param.comm = communicator;

    mlsl_sched_key key{};
    key.ctype = mlsl_coll_reduce;
    key.buf1 = (void*) send_buf;
    key.buf2 = recv_buf;
    key.count1 = count;
    key.dtype = dtype;
    key.reduction = reduction;
    key.root = root;
    key.comm = communicator;

    auto req = mlsl_coll_create(attributes, key, coll_param);
    LOG_DEBUG("coll ", mlsl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}

mlsl_request* mlsl_allreduce_impl(const void* send_buf,
                                  void* recv_buf,
                                  size_t count,
                                  mlsl_datatype_t dtype,
                                  mlsl_reduction_t reduction,
                                  const mlsl_coll_attr_t* attributes,
                                  mlsl_comm* communicator)
{
    mlsl_coll_param coll_param{};
    coll_param.ctype = mlsl_coll_allreduce;
    coll_param.send_buf = send_buf;
    coll_param.recv_buf = recv_buf;
    coll_param.count = count;
    coll_param.dtype = mlsl_datatype_get(dtype);
    coll_param.reduction = reduction;
    coll_param.comm = communicator;

    mlsl_sched_key key{};
    key.ctype = mlsl_coll_allreduce;
    key.buf1 = (void*) send_buf;
    key.buf2 = recv_buf;
    key.count1 = count;
    key.dtype = dtype;
    key.reduction = reduction;
    key.comm = communicator;

    auto req = mlsl_coll_create(attributes, key, coll_param);
    LOG_DEBUG("coll ", mlsl_coll_type_to_str(coll_param.ctype), " created, req ", req, " count ", count);
    return req;
}

mlsl_request* mlsl_allgatherv_impl(const void* send_buf,
                                   size_t send_count,
                                   void* recv_buf,
                                   size_t* recv_counts,
                                   mlsl_datatype_t dtype,
                                   const mlsl_coll_attr_t* attributes,
                                   mlsl_comm* communicator)
{
    mlsl_coll_param coll_param{};
    coll_param.ctype = mlsl_coll_allgatherv;
    coll_param.send_buf = send_buf;
    coll_param.recv_buf = recv_buf;
    coll_param.send_count = send_count;
    coll_param.recv_counts = recv_counts;
    coll_param.dtype = mlsl_datatype_get(dtype);
    coll_param.comm = communicator;

    mlsl_sched_key key{};
    key.ctype = mlsl_coll_allgatherv;
    key.buf1 = (void*) send_buf;
    key.buf2 = recv_buf;
    key.buf3 = recv_counts;
    key.count1 = send_count;
    key.comm = communicator;

    auto req = mlsl_coll_create(attributes, key, coll_param);
    LOG_DEBUG("coll ", mlsl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}

void mlsl_barrier_impl(mlsl_comm* communicator)
{
    mlsl_coll_attr_t attributes{};
    attributes.synchronous = 1;

    mlsl_coll_param coll_param{};
    coll_param.ctype = mlsl_coll_barrier;
    coll_param.dtype = mlsl_dtype_internal_char;
    coll_param.comm = communicator;

    mlsl_sched_key key{};
    key.ctype = mlsl_coll_barrier;
    key.comm = communicator;

    mlsl_coll_create(&attributes, key, coll_param);
}
