#include "iccl.hpp"
#include "coll/coll.hpp"
#include "coll/coll_algorithms.hpp"
#include "sched/sched_cache.hpp"
#include "common/request/request.hpp"
#include "common/utils/tree.hpp"
#include "out_of_order/ooo_match.hpp"
#include "fusion/fusion.hpp"

const char* iccl_coll_type_to_str(iccl_coll_type type)
{
    switch (type)
    {
        case iccl_coll_barrier:
            return "BARRIER";
        case iccl_coll_bcast:
            return "BCAST";
        case iccl_coll_reduce:
            return "REDUCE";
        case iccl_coll_allreduce:
            return "ALLREDUCE";
        case iccl_coll_allgatherv:
            return "ALLGATHERV";
        case iccl_coll_sparse_allreduce:
            return "SPARSE_ALLREDUCE";
        case iccl_coll_internal:
            return "INTERNAL";
        case iccl_coll_none:
            return "NONE";
        default:
            ICCL_FATAL("unexpected coll_type ", type);
            return "UNKNOWN";
    }
}

static iccl_request* iccl_coll_create(const iccl_coll_attr_t* attributes,
                                      iccl_sched_key& key,
                                      iccl_coll_param& coll_param)
{
    iccl_sched* sched = nullptr;
    iccl_comm* match_id_comm = nullptr;
    bool should_run = true;
    bool should_commit = false;
    bool should_cache = false;
    bool was_fused = false;
    std::string match_id;
    iccl_request* request = nullptr;
    const iccl_coll_attr_t* attr = attributes ? attributes : global_data.default_coll_attr.get();

    ICCL_THROW_IF_NOT(coll_param.ctype == iccl_coll_allreduce ||
                      !(attr->prologue_fn || attr->epilogue_fn || attr->reduction_fn),
                      "incorrect input");

    should_cache = attr->to_cache;

    if (attr->match_id && (env_data.out_of_order_support || should_cache))
        match_id.assign(attr->match_id);

    if (!match_id.empty() && env_data.out_of_order_support)
    {
        match_id_comm = global_data.ooo_manager->get_comm(match_id);
        if (!match_id_comm)
        {
            if (attr->synchronous)
            {
                ICCL_THROW("collective is synchronous and out-of-order is enabled but communicator is not found");
            }

            // user has provided match_id that has not been resolved yet.
            // schedule will be postponed until comm resolution
            should_run = false;
        }
        else
        {
            LOG_DEBUG("found comm id ", match_id_comm->id(), " for match_id ", match_id.c_str());
        }
    }

    if (should_cache && match_id.empty())
    {
        LOG_DEBUG("no match_id provided, disabling collective caching");
        should_cache = false;
    }

    if (should_cache)
    {
        key.prologue_fn = attr->prologue_fn;
        key.epilogue_fn = attr->epilogue_fn;
        key.reduction_fn = attr->reduction_fn;
        key.priority = attr->priority;
        key.synchronous = attr->synchronous;
        key.match_id = match_id;
        sched = global_data.sched_cache->find(key);
    }

    if (!sched)
    {
        sched = new iccl_sched(coll_param);
        LOG_DEBUG("didn't find sched, create new one ", sched, ", type ",
                  iccl_coll_type_to_str(sched->coll_param.ctype));

        if (should_cache)
        {
            global_data.sched_cache->add(key, sched);
        }

        sched->set_coll_attr(attr, std::move(match_id));
        sched->coll_attr.to_cache = should_cache;
        should_commit = true;
    }
    else
    {
        /* update buffer parameters in existing schedule
           as they could be changed since previous call */

        sched->coll_param.buf = coll_param.buf;
        sched->coll_param.send_buf = coll_param.send_buf;
        sched->coll_param.recv_buf = coll_param.recv_buf;
        sched->coll_param.recv_counts = coll_param.recv_counts;

        if (coll_param.ctype == iccl_coll_sparse_allreduce)
        {
            sched->coll_param.sparse_param.snd_val_buf = coll_param.sparse_param.snd_val_buf;
            sched->coll_param.sparse_param.rcv_ind_buf = coll_param.sparse_param.rcv_ind_buf;
            sched->coll_param.sparse_param.rcv_val_buf = coll_param.sparse_param.rcv_val_buf;
        }

        LOG_DEBUG("found sched, reuse ", sched, ", type ",
                  iccl_coll_type_to_str(sched->coll_param.ctype));
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
                LOG_DEBUG("sched ", sched, ", ctype ", iccl_coll_type_to_str(sched->coll_param.ctype), "will be fused");
                request = sched->req;
                return request;
            }
        }
    }

    ICCL_ASSERT(!was_fused);

    if (should_commit)
    {
        sched->commit(global_data.parallelizer.get());
    }

    if (should_run)
    {
        // normal schedule execution - either with user-defined or match_id specific communicator
        request = sched->start(global_data.executor.get());
        if (attr->synchronous)
        {
            iccl_wait(request);
            request = nullptr;
        }
    }
    else
    {
        ICCL_ASSERT(!sched->coll_attr.match_id.empty(), "invalid match_id");

        request = sched->reset_request();
        LOG_INFO("sched ", sched, ", postponed for match_id resolution");
        global_data.ooo_manager->postpone(sched);
    }

    return request;
}

iccl_status_t iccl_coll_build_barrier(iccl_sched* sched)
{
    iccl_status_t status;
    sched->coll_param.ctype = iccl_coll_barrier;

    switch (env_data.barrier_algo)
    {
        case iccl_barrier_algo_ring:
            ICCL_CALL(iccl_coll_build_dissemination_barrier(sched));
            break;
        case iccl_barrier_algo_direct:
            ICCL_CALL(iccl_coll_build_direct_barrier(sched));
            break;
        default:
            ICCL_FATAL("unexpected barrier_algo ",
                       iccl_barrier_algo_to_str(env_data.barrier_algo));
            return iccl_status_invalid_arguments;
    }

    return status;
}

iccl_status_t iccl_coll_build_bcast(iccl_sched* sched,
                                    iccl_buffer buf,
                                    size_t count,
                                    iccl_datatype_internal_t dtype,
                                    size_t root)
{
    iccl_status_t status;
    sched->coll_param.ctype = iccl_coll_bcast;

    switch (env_data.bcast_algo)
    {
        case iccl_bcast_algo_ring:
            ICCL_CALL(iccl_coll_build_scatter_ring_allgather_bcast(sched, buf, count, dtype, root));
            break;
        case iccl_bcast_algo_double_tree:
            ICCL_CALL(iccl_coll_build_double_tree_op(sched, iccl_coll_bcast, iccl_buffer(), buf, count, dtype,
                                                     iccl_reduction_custom,
                                                     root == 0 ? sched->coll_param.comm->dtree() :
                                                     sched->coll_param.comm->dtree().copy_with_new_root(root)));
            break;
        case iccl_bcast_algo_direct:
            ICCL_CALL(iccl_coll_build_direct_bcast(sched, buf, count, dtype, root));
            break;
        default:
            ICCL_FATAL("unexpected bcast_algo ",
                       iccl_bcast_algo_to_str(env_data.bcast_algo));
            return iccl_status_invalid_arguments;
    }
    return status;
}

iccl_status_t iccl_coll_build_reduce(iccl_sched* sched,
                                     iccl_buffer send_buf,
                                     iccl_buffer recv_buf,
                                     size_t count,
                                     iccl_datatype_internal_t dtype,
                                     iccl_reduction_t reduction,
                                     size_t root)
{
    iccl_status_t status;
    sched->coll_param.ctype = iccl_coll_reduce;

    switch (env_data.reduce_algo)
    {
        case iccl_reduce_algo_tree:
            if (count < sched->coll_param.comm->pof2())
                ICCL_CALL(iccl_coll_build_binomial_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            else
                ICCL_CALL(iccl_coll_build_rabenseifner_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            break;
        case iccl_reduce_algo_double_tree:
            if (count < sched->coll_param.comm->pof2())
                ICCL_CALL(iccl_coll_build_binomial_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            else
                ICCL_CALL(iccl_coll_build_double_tree_op(sched, iccl_coll_reduce, send_buf, recv_buf, count, dtype,
                                                         reduction,
                                                         root == 0 ? sched->coll_param.comm->dtree() :
                                                         sched->coll_param.comm->dtree().copy_with_new_root(root)));
            break;
        case iccl_reduce_algo_direct:
            ICCL_CALL(iccl_coll_build_direct_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            break;
        default:
            ICCL_FATAL("unexpected reduce_algo ",
                       iccl_reduce_algo_to_str(env_data.reduce_algo));
            return iccl_status_invalid_arguments;
    }

    return status;
}

iccl_status_t iccl_coll_build_allgatherv(
    iccl_sched* sched,
    iccl_buffer send_buf,
    size_t s_count,
    iccl_buffer recv_buf,
    size_t* r_counts,
    iccl_datatype_internal_t dtype)
{
    iccl_status_t status;
    sched->coll_param.ctype = iccl_coll_allgatherv;

    switch (env_data.allgatherv_algo)
    {
        case iccl_allgatherv_algo_naive:
            ICCL_CALL(iccl_coll_build_naive_allgatherv(sched, send_buf, s_count, recv_buf, r_counts,
                                                       dtype));
            break;
        case iccl_allgatherv_algo_direct:
            ICCL_CALL(iccl_coll_build_direct_allgatherv(sched, send_buf, s_count, recv_buf, r_counts,
                                                       dtype));
            break;
        default:
            ICCL_FATAL("unexpected allgatherv_algo ",
                       iccl_allgatherv_algo_to_str(env_data.allgatherv_algo));
            return iccl_status_invalid_arguments;
    }
    return status;
}

iccl_status_t iccl_coll_build_allreduce(
    iccl_sched* sched,
    iccl_buffer send_buf,
    iccl_buffer recv_buf,
    size_t count,
    iccl_datatype_internal_t dtype,
    iccl_reduction_t reduction)
{
    iccl_status_t status = iccl_status_success;
    sched->coll_param.ctype = iccl_coll_allreduce;

    if ((count < sched->coll_param.comm->pof2()) || (count * iccl_datatype_get_size(dtype) <= 8192))
    {
        switch (env_data.allreduce_algo)
        {
            case iccl_allreduce_algo_ring:
                ICCL_CALL(iccl_coll_build_ring_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case iccl_allreduce_algo_ring_rma:
                if (global_data.executor->is_rma_enabled)
                    ICCL_CALL(iccl_coll_build_ring_rma_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                else
                    ICCL_CALL(iccl_coll_build_ring_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case iccl_allreduce_algo_direct:
                ICCL_CALL(iccl_coll_build_direct_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            default:
                ICCL_CALL(iccl_coll_build_recursive_doubling_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
        }
    }
    else
    {
        switch (env_data.allreduce_algo)
        {
            case iccl_allreduce_algo_rabenseifner:
                ICCL_CALL(iccl_coll_build_rabenseifner_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case iccl_allreduce_algo_starlike:
                ICCL_CALL(iccl_coll_build_starlike_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case iccl_allreduce_algo_ring:
                ICCL_CALL(iccl_coll_build_ring_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case iccl_allreduce_algo_ring_rma:
                if (global_data.executor->is_rma_enabled)
                    ICCL_CALL(iccl_coll_build_ring_rma_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                else
                    ICCL_CALL(iccl_coll_build_ring_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case iccl_allreduce_algo_double_tree:
                ICCL_CALL(
                    iccl_coll_build_double_tree_op(sched, iccl_coll_allreduce, send_buf, recv_buf,
                                                   count, dtype, reduction, sched->coll_param.comm->dtree()));
                break;
            case iccl_allreduce_algo_direct:
                ICCL_CALL(iccl_coll_build_direct_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            default:
                ICCL_FATAL("unexpected allreduce_algo ",
                           iccl_allreduce_algo_to_str(env_data.allreduce_algo));
                return iccl_status_invalid_arguments;
        }
    }

    return status;
}

iccl_status_t iccl_coll_build_sparse_allreduce(
    iccl_sched* sched,
    iccl_buffer send_ind_buf, size_t send_ind_count,
    iccl_buffer send_val_buf, size_t send_val_count,
    iccl_buffer recv_ind_buf, size_t* recv_ind_count,
    iccl_buffer recv_val_buf, size_t* recv_val_count,
    iccl_datatype_internal_t index_dtype,
    iccl_datatype_internal_t value_dtype,
    iccl_reduction_t reduction)
{
    iccl_status_t status;
    sched->coll_param.ctype = iccl_coll_sparse_allreduce;

    switch (env_data.sparse_allreduce_algo)
    {
        case iccl_sparse_allreduce_algo_basic:
            ICCL_CALL(iccl_coll_build_sparse_allreduce_basic(sched,
                                                             send_ind_buf, send_ind_count,
                                                             send_val_buf, send_val_count,
                                                             recv_ind_buf, recv_ind_count,
                                                             recv_val_buf, recv_val_count,
                                                             index_dtype, value_dtype, reduction));
            break;
        default:
            ICCL_FATAL("unexpected sparse_allreduce_algo ", env_data.sparse_allreduce_algo);
            return iccl_status_invalid_arguments;
    }

    return status;
}

iccl_request* iccl_bcast_impl(void* buf,
                              size_t count,
                              iccl_datatype_t dtype,
                              size_t root,
                              const iccl_coll_attr_t* attributes,
                              iccl_comm* communicator)
{
    iccl_coll_param coll_param{};
    coll_param.ctype = iccl_coll_bcast;
    coll_param.buf = buf;
    coll_param.count = count;
    coll_param.dtype = iccl_datatype_get(dtype);
    coll_param.root = root;
    coll_param.comm = communicator;

    iccl_sched_key key{};
    key.ctype = iccl_coll_bcast;
    key.count1 = count;
    key.dtype = dtype;
    key.root = root;
    key.comm = communicator;

    auto req = iccl_coll_create(attributes, key, coll_param);
    LOG_DEBUG("coll ", iccl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}

iccl_request* iccl_reduce_impl(const void* send_buf,
                               void* recv_buf,
                               size_t count,
                               iccl_datatype_t dtype,
                               iccl_reduction_t reduction,
                               size_t root,
                               const iccl_coll_attr_t* attributes,
                               iccl_comm* communicator)
{
    iccl_coll_param coll_param{};
    coll_param.ctype = iccl_coll_reduce;
    coll_param.send_buf = send_buf;
    coll_param.recv_buf = recv_buf;
    coll_param.count = count;
    coll_param.dtype = iccl_datatype_get(dtype);
    coll_param.reduction = reduction;
    coll_param.root = root;
    coll_param.comm = communicator;

    iccl_sched_key key{};
    key.ctype = iccl_coll_reduce;
    key.count1 = count;
    key.dtype = dtype;
    key.reduction = reduction;
    key.root = root;
    key.comm = communicator;

    auto req = iccl_coll_create(attributes, key, coll_param);
    LOG_DEBUG("coll ", iccl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}

iccl_request* iccl_allreduce_impl(const void* send_buf,
                                  void* recv_buf,
                                  size_t count,
                                  iccl_datatype_t dtype,
                                  iccl_reduction_t reduction,
                                  const iccl_coll_attr_t* attributes,
                                  iccl_comm* communicator)
{
    iccl_coll_param coll_param{};
    coll_param.ctype = iccl_coll_allreduce;
    coll_param.send_buf = send_buf;
    coll_param.recv_buf = recv_buf;
    coll_param.count = count;
    coll_param.dtype = iccl_datatype_get(dtype);
    coll_param.reduction = reduction;
    coll_param.comm = communicator;

    iccl_sched_key key{};
    key.ctype = iccl_coll_allreduce;
    key.count1 = count;
    key.dtype = dtype;
    key.reduction = reduction;
    key.comm = communicator;

    auto req = iccl_coll_create(attributes, key, coll_param);
    LOG_DEBUG("coll ", iccl_coll_type_to_str(coll_param.ctype), " created, req ", req, " count ", count);
    return req;
}

iccl_request* iccl_allgatherv_impl(const void* send_buf,
                                   size_t send_count,
                                   void* recv_buf,
                                   size_t* recv_counts,
                                   iccl_datatype_t dtype,
                                   const iccl_coll_attr_t* attributes,
                                   iccl_comm* communicator)
{
    iccl_coll_param coll_param{};
    coll_param.ctype = iccl_coll_allgatherv;
    coll_param.send_buf = send_buf;
    coll_param.recv_buf = recv_buf;
    coll_param.send_count = send_count;
    coll_param.recv_counts = recv_counts;
    coll_param.dtype = iccl_datatype_get(dtype);
    coll_param.comm = communicator;

    iccl_sched_key key{};
    key.ctype = iccl_coll_allgatherv;
    key.buf = recv_counts;
    key.count1 = send_count;
    key.dtype = dtype;
    key.comm = communicator;

    auto req = iccl_coll_create(attributes, key, coll_param);
    LOG_DEBUG("coll ", iccl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}

void iccl_barrier_impl(iccl_comm* communicator)
{
    iccl_coll_attr_t attributes{};
    attributes.synchronous = 1;

    iccl_coll_param coll_param{};
    coll_param.ctype = iccl_coll_barrier;
    coll_param.dtype = iccl_dtype_internal_char;
    coll_param.comm = communicator;

    iccl_sched_key key{};
    key.ctype = iccl_coll_barrier;
    key.comm = communicator;

    iccl_coll_create(&attributes, key, coll_param);
}

iccl_request* iccl_sparse_allreduce_impl(const void* send_ind_buf, size_t send_ind_count,
                                         const void* send_val_buf, size_t send_val_count,
                                         void** recv_ind_buf, size_t* recv_ind_count,
                                         void** recv_val_buf, size_t* recv_val_count,
                                         iccl_datatype_t index_dtype,
                                         iccl_datatype_t dtype,
                                         iccl_reduction_t reduction,
                                         const iccl_coll_attr_t* attributes,
                                         iccl_comm* communicator)
{
    iccl_coll_param coll_param{};
    coll_param.ctype = iccl_coll_sparse_allreduce;
    coll_param.send_buf = send_ind_buf;
    coll_param.send_count = send_ind_count;
    coll_param.sparse_param.snd_val_buf = send_val_buf;
    coll_param.sparse_param.snd_val_count = send_val_count;
    coll_param.sparse_param.rcv_ind_buf = recv_ind_buf;
    coll_param.recv_counts = recv_ind_count;
    coll_param.sparse_param.rcv_val_buf = recv_val_buf;
    coll_param.sparse_param.rcv_val_count = recv_val_count;
    coll_param.dtype = iccl_datatype_get(dtype);
    coll_param.sparse_param.itype = iccl_datatype_get(index_dtype);
    coll_param.reduction = reduction;
    coll_param.comm = communicator;

    iccl_sched_key key{};
    key.ctype = iccl_coll_sparse_allreduce;
    key.count1 = send_ind_count;
    key.count2 = send_val_count;
    key.count3 = recv_ind_count;
    key.count4 = recv_val_count;
    key.itype = index_dtype;
    key.dtype = dtype;
    key.reduction = reduction;
    key.comm = communicator;

    auto req = iccl_coll_create(attributes, key, coll_param);
    LOG_DEBUG("coll ", iccl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}
