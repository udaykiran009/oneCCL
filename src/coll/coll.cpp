#include "ccl.hpp"
#include "coll/coll.hpp"
#include "coll/coll_algorithms.hpp"
#include "sched/sched_cache.hpp"
#include "common/request/request.hpp"
#include "common/utils/tree.hpp"
#include "out_of_order/ooo_match.hpp"
#include "fusion/fusion.hpp"

const char* ccl_coll_type_to_str(ccl_coll_type type)
{
    switch (type)
    {
        case ccl_coll_barrier:
            return "BARRIER";
        case ccl_coll_bcast:
            return "BCAST";
        case ccl_coll_reduce:
            return "REDUCE";
        case ccl_coll_allreduce:
            return "ALLREDUCE";
        case ccl_coll_allgatherv:
            return "ALLGATHERV";
        case ccl_coll_sparse_allreduce:
            return "SPARSE_ALLREDUCE";
        case ccl_coll_internal:
            return "INTERNAL";
        case ccl_coll_none:
            return "NONE";
        default:
            CCL_FATAL("unexpected coll_type ", type);
            return "UNKNOWN";
    }
}

static ccl_request* ccl_coll_create(ccl_coll_param& coll_param,
                                    const ccl_coll_attr_t* attributes,
                                    ccl_sched_key& key)
{
    ccl_sched* sched = nullptr;
    ccl_comm* match_id_comm = nullptr;
    ccl_request* request = nullptr;
    const ccl_coll_attr_t* attr = attributes ? attributes : global_data.default_coll_attr.get();

    bool should_run = true;
    bool should_cache = false;
    bool was_fused = false;

    std::string match_id;

    CCL_THROW_IF_NOT(coll_param.ctype == ccl_coll_allreduce ||
                     !(attr->prologue_fn || attr->epilogue_fn || attr->reduction_fn),
                     "for now only allreduce supports prologue/epilogue/custom_reduction functionality");

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
                CCL_THROW("unsupported case (synchronous && out-of-order && !communicator)");
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
        key.match_id = match_id;
        sched = global_data.sched_cache->find(key);
    }

    if (!sched)
    {
        sched = new ccl_sched(coll_param);
        LOG_DEBUG("didn't find sched, create new one ", sched, ", type ",
                  ccl_coll_type_to_str(sched->coll_param.ctype));

        if (should_cache)
        {
            global_data.sched_cache->add(key, sched);
        }

        sched->set_coll_attr(attr, std::move(match_id));
        sched->coll_attr.to_cache = should_cache;
    }
    else
    {
        /* update some parameters and attributes in existing schedule
           as they could be changed since previous call */
        sched->update_coll_param(coll_param);
        sched->update_coll_attr(attr);

        LOG_DEBUG("found sched, reuse ", sched, ", type ",
                  ccl_coll_type_to_str(sched->coll_param.ctype));
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
                LOG_DEBUG("sched ", sched, ", ctype ", ccl_coll_type_to_str(sched->coll_param.ctype), "will be fused");
                request = sched->req;
                return request;
            }
        }
    }

    CCL_ASSERT(!was_fused);

    sched->commit(global_data.parallelizer.get());

    if (should_run)
    {
        // normal schedule execution - either with user-defined or match_id specific communicator
        request = sched->start(global_data.executor.get());
        if (attr->synchronous)
        {
            ccl_wait(request);
            request = nullptr;
        }
    }
    else
    {
        CCL_ASSERT(!sched->coll_attr.match_id.empty(), "invalid match_id");

        request = sched->reset_request();
        LOG_INFO("sched ", sched, ", postponed for match_id resolution");
        global_data.ooo_manager->postpone(sched);
    }

    return request;
}

ccl_status_t ccl_coll_build_barrier(ccl_sched* sched)
{
    ccl_status_t status;
    sched->coll_param.ctype = ccl_coll_barrier;

    switch (env_data.barrier_algo)
    {
        case ccl_barrier_algo_ring:
            CCL_CALL(ccl_coll_build_dissemination_barrier(sched));
            break;
        case ccl_barrier_algo_direct:
            CCL_CALL(ccl_coll_build_direct_barrier(sched));
            break;
        default:
            CCL_FATAL("unexpected barrier_algo ",
                       ccl_barrier_algo_to_str(env_data.barrier_algo));
            return ccl_status_invalid_arguments;
    }

    return status;
}

ccl_status_t ccl_coll_build_bcast(ccl_sched* sched,
                                  ccl_buffer buf,
                                  size_t count,
                                  ccl_datatype_internal_t dtype,
                                  size_t root)
{
    ccl_status_t status;
    sched->coll_param.ctype = ccl_coll_bcast;

    switch (env_data.bcast_algo)
    {
        case ccl_bcast_algo_ring:
            CCL_CALL(ccl_coll_build_scatter_ring_allgather_bcast(sched, buf, count, dtype, root));
            break;
        case ccl_bcast_algo_double_tree:
            CCL_CALL(ccl_coll_build_double_tree_op(sched, ccl_coll_bcast, ccl_buffer(), buf, count, dtype,
                                                   ccl_reduction_custom,
                                                   root == 0 ? sched->coll_param.comm->dtree() :
                                                   sched->coll_param.comm->dtree().copy_with_new_root(root)));
            break;
        case ccl_bcast_algo_direct:
            CCL_CALL(ccl_coll_build_direct_bcast(sched, buf, count, dtype, root));
            break;
        default:
            CCL_FATAL("unexpected bcast_algo ",
                       ccl_bcast_algo_to_str(env_data.bcast_algo));
            return ccl_status_invalid_arguments;
    }
    return status;
}

ccl_status_t ccl_coll_build_reduce(ccl_sched* sched,
                                   ccl_buffer send_buf,
                                   ccl_buffer recv_buf,
                                   size_t count,
                                   ccl_datatype_internal_t dtype,
                                   ccl_reduction_t reduction,
                                   size_t root)
{
    ccl_status_t status;
    sched->coll_param.ctype = ccl_coll_reduce;

    switch (env_data.reduce_algo)
    {
        case ccl_reduce_algo_tree:
            if (count < sched->coll_param.comm->pof2())
                CCL_CALL(ccl_coll_build_binomial_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            else
                CCL_CALL(ccl_coll_build_rabenseifner_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            break;
        case ccl_reduce_algo_double_tree:
            if (count < sched->coll_param.comm->pof2())
                CCL_CALL(ccl_coll_build_binomial_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            else
                CCL_CALL(ccl_coll_build_double_tree_op(sched, ccl_coll_reduce, send_buf, recv_buf, count, dtype,
                                                       reduction,
                                                       root == 0 ? sched->coll_param.comm->dtree() :
                                                       sched->coll_param.comm->dtree().copy_with_new_root(root)));
            break;
        case ccl_reduce_algo_direct:
            CCL_CALL(ccl_coll_build_direct_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            break;
        default:
            CCL_FATAL("unexpected reduce_algo ",
                       ccl_reduce_algo_to_str(env_data.reduce_algo));
            return ccl_status_invalid_arguments;
    }

    return status;
}

ccl_status_t ccl_coll_build_allgatherv(
    ccl_sched* sched,
    ccl_buffer send_buf,
    size_t s_count,
    ccl_buffer recv_buf,
    size_t* r_counts,
    ccl_datatype_internal_t dtype)
{
    ccl_status_t status;
    sched->coll_param.ctype = ccl_coll_allgatherv;

    switch (env_data.allgatherv_algo)
    {
        case ccl_allgatherv_algo_naive:
            CCL_CALL(ccl_coll_build_naive_allgatherv(sched, send_buf, s_count, recv_buf, r_counts,
                                                     dtype));
            break;
        case ccl_allgatherv_algo_direct:
            CCL_CALL(ccl_coll_build_direct_allgatherv(sched, send_buf, s_count, recv_buf, r_counts,
                                                      dtype));
            break;
        default:
            CCL_FATAL("unexpected allgatherv_algo ",
                       ccl_allgatherv_algo_to_str(env_data.allgatherv_algo));
            return ccl_status_invalid_arguments;
    }
    return status;
}

ccl_status_t ccl_coll_build_allreduce(
    ccl_sched* sched,
    ccl_buffer send_buf,
    ccl_buffer recv_buf,
    size_t count,
    ccl_datatype_internal_t dtype,
    ccl_reduction_t reduction)
{
    ccl_status_t status = ccl_status_success;
    sched->coll_param.ctype = ccl_coll_allreduce;

    if ((count < sched->coll_param.comm->pof2()) || (count * ccl_datatype_get_size(dtype) <= 8192))
    {
        switch (env_data.allreduce_algo)
        {
            case ccl_allreduce_algo_ring:
                CCL_CALL(ccl_coll_build_ring_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case ccl_allreduce_algo_ring_rma:
                if (global_data.executor->is_rma_enabled)
                    CCL_CALL(ccl_coll_build_ring_rma_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                else
                    CCL_CALL(ccl_coll_build_ring_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case ccl_allreduce_algo_direct:
                CCL_CALL(ccl_coll_build_direct_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            default:
                CCL_CALL(ccl_coll_build_recursive_doubling_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
        }
    }
    else
    {
        switch (env_data.allreduce_algo)
        {
            case ccl_allreduce_algo_rabenseifner:
                CCL_CALL(ccl_coll_build_rabenseifner_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case ccl_allreduce_algo_starlike:
                CCL_CALL(ccl_coll_build_starlike_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case ccl_allreduce_algo_ring:
                CCL_CALL(ccl_coll_build_ring_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case ccl_allreduce_algo_ring_rma:
                if (global_data.executor->is_rma_enabled)
                    CCL_CALL(ccl_coll_build_ring_rma_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                else
                    CCL_CALL(ccl_coll_build_ring_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            case ccl_allreduce_algo_double_tree:
                CCL_CALL(
                    ccl_coll_build_double_tree_op(sched, ccl_coll_allreduce, send_buf, recv_buf,
                                                  count, dtype, reduction, sched->coll_param.comm->dtree()));
                break;
            case ccl_allreduce_algo_direct:
                CCL_CALL(ccl_coll_build_direct_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
                break;
            default:
                CCL_FATAL("unexpected allreduce_algo ",
                           ccl_allreduce_algo_to_str(env_data.allreduce_algo));
                return ccl_status_invalid_arguments;
        }
    }

    return status;
}

ccl_status_t ccl_coll_build_sparse_allreduce(
    ccl_sched* sched,
    ccl_buffer send_ind_buf, size_t send_ind_count,
    ccl_buffer send_val_buf, size_t send_val_count,
    ccl_buffer recv_ind_buf, size_t* recv_ind_count,
    ccl_buffer recv_val_buf, size_t* recv_val_count,
    ccl_datatype_internal_t index_dtype,
    ccl_datatype_internal_t value_dtype,
    ccl_reduction_t reduction)
{
    ccl_status_t status;
    sched->coll_param.ctype = ccl_coll_sparse_allreduce;

    switch (env_data.sparse_allreduce_algo)
    {
        case ccl_sparse_allreduce_algo_basic:
            CCL_CALL(ccl_coll_build_sparse_allreduce_basic(sched,
                                                           send_ind_buf, send_ind_count,
                                                           send_val_buf, send_val_count,
                                                           recv_ind_buf, recv_ind_count,
                                                           recv_val_buf, recv_val_count,
                                                           index_dtype, value_dtype, reduction));
            break;
        default:
            CCL_FATAL("unexpected sparse_allreduce_algo ", env_data.sparse_allreduce_algo);
            return ccl_status_invalid_arguments;
    }

    return status;
}

ccl_request* ccl_bcast_impl(void* buf,
                            size_t count,
                            ccl_datatype_t dtype,
                            size_t root,
                            const ccl_coll_attr_t* attributes,
                            ccl_comm* communicator)
{
    ccl_coll_param coll_param{};
    coll_param.ctype = ccl_coll_bcast;
    coll_param.buf = buf;
    coll_param.count = count;
    coll_param.dtype = ccl_datatype_get(dtype);
    coll_param.root = root;
    coll_param.comm = communicator;

    ccl_sched_key key{};
    key.ctype = ccl_coll_bcast;
    key.count1 = count;
    key.dtype = dtype;
    key.root = root;
    key.comm = communicator;

    auto req = ccl_coll_create(coll_param, attributes, key);
    LOG_DEBUG("coll ", ccl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}

ccl_request* ccl_reduce_impl(const void* send_buf,
                             void* recv_buf,
                             size_t count,
                             ccl_datatype_t dtype,
                             ccl_reduction_t reduction,
                             size_t root,
                             const ccl_coll_attr_t* attributes,
                             ccl_comm* communicator)
{
    ccl_coll_param coll_param{};
    coll_param.ctype = ccl_coll_reduce;
    coll_param.send_buf = send_buf;
    coll_param.recv_buf = recv_buf;
    coll_param.count = count;
    coll_param.dtype = ccl_datatype_get(dtype);
    coll_param.reduction = reduction;
    coll_param.root = root;
    coll_param.comm = communicator;

    ccl_sched_key key{};
    key.ctype = ccl_coll_reduce;
    key.count1 = count;
    key.dtype = dtype;
    key.reduction = reduction;
    key.root = root;
    key.comm = communicator;

    auto req = ccl_coll_create(coll_param, attributes, key);
    LOG_DEBUG("coll ", ccl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}

ccl_request* ccl_allreduce_impl(const void* send_buf,
                                void* recv_buf,
                                size_t count,
                                ccl_datatype_t dtype,
                                ccl_reduction_t reduction,
                                const ccl_coll_attr_t* attributes,
                                ccl_comm* communicator)
{
    ccl_coll_param coll_param{};
    coll_param.ctype = ccl_coll_allreduce;
    coll_param.send_buf = send_buf;
    coll_param.recv_buf = recv_buf;
    coll_param.count = count;
    coll_param.dtype = ccl_datatype_get(dtype);
    coll_param.reduction = reduction;
    coll_param.comm = communicator;

    ccl_sched_key key{};
    key.ctype = ccl_coll_allreduce;
    key.count1 = count;
    key.dtype = dtype;
    key.reduction = reduction;
    key.comm = communicator;

    auto req = ccl_coll_create(coll_param, attributes, key);
    LOG_DEBUG("coll ", ccl_coll_type_to_str(coll_param.ctype), " created, req ", req, " count ", count);
    return req;
}

ccl_request* ccl_allgatherv_impl(const void* send_buf,
                                 size_t send_count,
                                 void* recv_buf,
                                 size_t* recv_counts,
                                 ccl_datatype_t dtype,
                                 const ccl_coll_attr_t* attributes,
                                 ccl_comm* communicator)
{
    ccl_coll_param coll_param{};
    coll_param.ctype = ccl_coll_allgatherv;
    coll_param.send_buf = send_buf;
    coll_param.recv_buf = recv_buf;
    coll_param.send_count = send_count;
    coll_param.recv_counts = recv_counts;
    coll_param.dtype = ccl_datatype_get(dtype);
    coll_param.comm = communicator;

    ccl_sched_key key{};
    key.ctype = ccl_coll_allgatherv;
    key.buf = recv_counts;
    key.count1 = send_count;
    key.dtype = dtype;
    key.comm = communicator;

    auto req = ccl_coll_create(coll_param, attributes, key);
    LOG_DEBUG("coll ", ccl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}

void ccl_barrier_impl(ccl_comm* communicator)
{
    ccl_coll_param coll_param{};
    coll_param.ctype = ccl_coll_barrier;
    coll_param.dtype = ccl_dtype_internal_char;
    coll_param.comm = communicator;

    ccl_coll_attr_t attributes{};
    attributes.synchronous = 1;

    ccl_sched_key key{};
    key.ctype = ccl_coll_barrier;
    key.comm = communicator;

    ccl_coll_create(coll_param, &attributes, key);
}

ccl_request* ccl_sparse_allreduce_impl(const void* send_ind_buf, size_t send_ind_count,
                                       const void* send_val_buf, size_t send_val_count,
                                       void** recv_ind_buf, size_t* recv_ind_count,
                                       void** recv_val_buf, size_t* recv_val_count,
                                       ccl_datatype_t index_dtype,
                                       ccl_datatype_t dtype,
                                       ccl_reduction_t reduction,
                                       const ccl_coll_attr_t* attributes,
                                       ccl_comm* communicator)
{
    ccl_coll_param coll_param{};
    coll_param.ctype = ccl_coll_sparse_allreduce;
    coll_param.send_buf = send_ind_buf;
    coll_param.send_count = send_ind_count;
    coll_param.sparse_param.snd_val_buf = send_val_buf;
    coll_param.sparse_param.snd_val_count = send_val_count;
    coll_param.sparse_param.rcv_ind_buf = recv_ind_buf;
    coll_param.recv_counts = recv_ind_count;
    coll_param.sparse_param.rcv_val_buf = recv_val_buf;
    coll_param.sparse_param.rcv_val_count = recv_val_count;
    coll_param.dtype = ccl_datatype_get(dtype);
    coll_param.sparse_param.itype = ccl_datatype_get(index_dtype);
    coll_param.reduction = reduction;
    coll_param.comm = communicator;

    ccl_sched_key key{};
    key.ctype = ccl_coll_sparse_allreduce;
    key.count1 = send_ind_count;
    key.count2 = send_val_count;
    key.count3 = recv_ind_count;
    key.count4 = recv_val_count;
    key.itype = index_dtype;
    key.dtype = dtype;
    key.reduction = reduction;
    key.comm = communicator;

    auto req = ccl_coll_create(coll_param, attributes, key);
    LOG_DEBUG("coll ", ccl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}
