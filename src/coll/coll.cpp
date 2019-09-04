#include "ccl.hpp"
#include "coll/algorithms/algorithms.hpp"
#include "coll/coll.hpp"
#include "coll/selection/selection.hpp"
#include "common/global/global.hpp"
#include "common/request/request.hpp"
#include "exec/exec.hpp"
#include "fusion/fusion.hpp"
#include "unordered_coll/unordered_coll.hpp"

bool is_attr_cached(const ccl_coll_attr_t& c_attr) noexcept
{
    return (c_attr.to_cache and c_attr.match_id and c_attr.match_id[0]);
}

ccl_coll_attr::ccl_coll_attr(const ccl_coll_attr_t& c_attr) :
  prologue_fn(c_attr.prologue_fn),
  epilogue_fn(c_attr.epilogue_fn),
  reduction_fn(c_attr.reduction_fn),
  priority(c_attr.priority),
  synchronous(c_attr.synchronous),
  to_cache(c_attr.to_cache),
  match_id(c_attr.match_id ? c_attr.match_id : "")
{
    to_cache = is_attr_cached(c_attr);
}

ccl_coll_attr& ccl_coll_attr::operator= (const ccl_coll_attr_t& c_attr)
{
  prologue_fn = c_attr.prologue_fn;
  epilogue_fn = c_attr.epilogue_fn;
  reduction_fn = c_attr.reduction_fn;
  priority = c_attr.priority;
  synchronous = c_attr.synchronous;
  to_cache = is_attr_cached(c_attr);
  match_id = (c_attr.match_id ? c_attr.match_id : "");
  return *this;
}

const char* ccl_coll_type_to_str(ccl_coll_type type)
{
    switch (type)
    {
        case ccl_coll_allgatherv:
            return "allgatherv";
        case ccl_coll_allreduce:
            return "allreduce";
        case ccl_coll_barrier:
            return "barrier";
        case ccl_coll_bcast:
            return "bcast";
        case ccl_coll_reduce:
            return "reduce";
        case ccl_coll_sparse_allreduce:
            return "sparse_allreduce";
        case ccl_coll_internal:
            return "internal";
        default:
            CCL_FATAL("unexpected coll_type ", type);
            return "unknown";
    }
}

static ccl_request* ccl_coll_create(ccl_coll_param& coll_param,
                                    const ccl_coll_attr_t* coll_attr,
                                    ccl_sched_key&& key)
{
    // get particular schedule: create new or take from cache
    ccl_master_sched* sched = ccl_master_sched::create(coll_param, coll_attr, std::move(key));
    // no use 'key' anymore, because it's moved

    // TODO move following pipeline into executor->start

    // 1. Check unordered first
    ccl_comm* match_id_comm = nullptr;
    if (!sched->coll_attr.match_id.empty() && env_data.enable_unordered_coll)
    {
        match_id_comm = global_data.unordered_coll_manager->get_comm(sched->coll_attr.match_id).get();
        if (!match_id_comm)
        {
            if (sched->coll_attr.synchronous)
            {
                CCL_THROW("unsupported collective (synchronous && unordered && !communicator)");
            }

            // user has provided match_id that has not been resolved yet.
            // schedule will be postponed until comm resolution
            // parallelize schedule
            sched->commit(global_data.parallelizer.get());
            return global_data.unordered_coll_manager->postpone(sched);
        }
        else
        {
            LOG_DEBUG("found comm id ", match_id_comm->id(), " for match_id ", sched->coll_attr.match_id);
        }
    }

    if (match_id_comm)
    {
        sched->coll_param.comm = match_id_comm;
    }

    // 2. then check fusion
    if (env_data.enable_fusion)
    {
        if (global_data.fusion_manager->add(sched))
        {
            LOG_DEBUG("sched ", sched, ", ctype ",
                      ccl_coll_type_to_str(sched->coll_param.ctype), " will be fused");
            return sched;
        }
    }


    // parallelize schedule
    sched->commit(global_data.parallelizer.get());

    // 3. regular schedule execution - either with user-defined or match_id specific communicator
    ccl_request* request = sched->start(global_data.executor.get());
    if (sched->coll_attr.synchronous)
    {
        ccl_wait_impl<ccl_master_sched>(global_data.executor.get(), request);
        request = nullptr;
    }

    return request;
}

ccl_status_t ccl_coll_build_allgatherv(
    ccl_sched* sched,
    ccl_buffer send_buf,
    size_t send_count,
    ccl_buffer recv_buf,
    const size_t* recv_counts,
    ccl_datatype_internal_t dtype)
{
    ccl_status_t status = ccl_status_success;

    sched->coll_param.ctype = ccl_coll_allgatherv;
    sched->coll_param.send_count = send_count;
    sched->coll_param.recv_counts = recv_counts;

    auto algo = global_data.algorithm_selector->get<ccl_coll_allgatherv>(sched->coll_param);

    switch (algo)
    {
        case ccl_coll_allgatherv_direct:
            CCL_CALL(ccl_coll_build_direct_allgatherv(sched, send_buf, send_count, recv_buf, recv_counts,
                                                      dtype));
            break;
        case ccl_coll_allgatherv_naive:
            CCL_CALL(ccl_coll_build_naive_allgatherv(sched, send_buf, send_count, recv_buf, recv_counts,
                                                     dtype));
            break;
        default:
            CCL_FATAL("unexpected allgatherv_algo ", ccl_coll_algorithm_to_str(algo));
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
    sched->coll_param.count = count;

    auto algo = global_data.algorithm_selector->get<ccl_coll_allreduce>(sched->coll_param);

    switch (algo)
    {
        case ccl_coll_allreduce_direct:
            CCL_CALL(ccl_coll_build_direct_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
            break;
        case ccl_coll_allreduce_rabenseifner:
            CCL_CALL(ccl_coll_build_rabenseifner_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
            break;
        case ccl_coll_allreduce_starlike:
            CCL_CALL(ccl_coll_build_starlike_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
            break;
        case ccl_coll_allreduce_ring:
            CCL_CALL(ccl_coll_build_ring_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
            break;
        case ccl_coll_allreduce_ring_rma:
            CCL_CALL(ccl_coll_build_ring_rma_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
            break;
        case ccl_coll_allreduce_double_tree:
            CCL_CALL(
                ccl_coll_build_double_tree_op(sched, ccl_coll_allreduce, send_buf, recv_buf,
                                              count, dtype, reduction, sched->coll_param.comm->dtree()));
            break;
        case ccl_coll_allreduce_recursive_doubling:
            CCL_CALL(ccl_coll_build_recursive_doubling_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
            break;
        default:
            CCL_FATAL("unexpected allreduce_algo ", ccl_coll_algorithm_to_str(algo));
            return ccl_status_invalid_arguments;
    }

    return status;
}

ccl_status_t ccl_coll_build_barrier(ccl_sched* sched)
{
    ccl_status_t status = ccl_status_success;

    sched->coll_param.ctype = ccl_coll_barrier;

    auto algo = global_data.algorithm_selector->get<ccl_coll_barrier>(sched->coll_param);

    switch (algo)
    {
        case ccl_coll_barrier_direct:
            CCL_CALL(ccl_coll_build_direct_barrier(sched));
            break;
        case ccl_coll_barrier_ring:
            CCL_CALL(ccl_coll_build_dissemination_barrier(sched));
            break;
        default:
            CCL_FATAL("unexpected barrier_algo ", ccl_coll_algorithm_to_str(algo));
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
    ccl_status_t status = ccl_status_success;

    sched->coll_param.ctype = ccl_coll_bcast;
    sched->coll_param.count = count;

    auto algo = global_data.algorithm_selector->get<ccl_coll_bcast>(sched->coll_param);

    switch (algo)
    {
        case ccl_coll_bcast_direct:
            CCL_CALL(ccl_coll_build_direct_bcast(sched, buf, count, dtype, root));
            break;
        case ccl_coll_bcast_ring:
            CCL_CALL(ccl_coll_build_scatter_ring_allgather_bcast(sched, buf, count, dtype, root));
            break;
        case ccl_coll_bcast_double_tree:
            CCL_CALL(ccl_coll_build_double_tree_op(sched, ccl_coll_bcast, ccl_buffer(), buf, count, dtype,
                                                   ccl_reduction_custom,
                                                   root == 0 ? sched->coll_param.comm->dtree() :
                                                   sched->coll_param.comm->dtree().copy_with_new_root(root)));
            break;
        case ccl_coll_bcast_naive:
            CCL_CALL(ccl_coll_build_naive_bcast(sched, buf, count, dtype, root));
            break;
        default:
            CCL_FATAL("unexpected bcast_algo ", ccl_coll_algorithm_to_str(algo));
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
    ccl_status_t status = ccl_status_success;

    sched->coll_param.ctype = ccl_coll_reduce;
    sched->coll_param.count = count;

    auto algo = global_data.algorithm_selector->get<ccl_coll_reduce>(sched->coll_param);

    switch (algo)
    {
        case ccl_coll_reduce_direct:
            CCL_CALL(ccl_coll_build_direct_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            break;
        case ccl_coll_reduce_rabenseifner:
            CCL_CALL(ccl_coll_build_rabenseifner_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            break;
        case ccl_coll_reduce_tree:
            CCL_CALL(ccl_coll_build_binomial_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
            break;
        case ccl_coll_reduce_double_tree:
            CCL_CALL(ccl_coll_build_double_tree_op(sched, ccl_coll_reduce, send_buf, recv_buf, count, dtype,
                                                   reduction,
                                                   root == 0 ? sched->coll_param.comm->dtree() :
                                                   sched->coll_param.comm->dtree().copy_with_new_root(root)));
            break;
        default:
            CCL_FATAL("unexpected reduce_algo ", ccl_coll_algorithm_to_str(algo));
            return ccl_status_invalid_arguments;
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
    ccl_status_t status = ccl_status_success;

    sched->coll_param.ctype = ccl_coll_sparse_allreduce;
    sched->coll_param.sparse_param.send_val_count = send_val_count;

    auto algo = global_data.algorithm_selector->get<ccl_coll_sparse_allreduce>(sched->coll_param);

    switch (algo)
    {
        case ccl_coll_sparse_allreduce_basic:
            CCL_CALL(ccl_coll_build_sparse_allreduce_basic(sched,
                                                           send_ind_buf, send_ind_count,
                                                           send_val_buf, send_val_count,
                                                           recv_ind_buf, recv_ind_count,
                                                           recv_val_buf, recv_val_count,
                                                           index_dtype, value_dtype, reduction));
            break;
        default:
            CCL_FATAL("unexpected sparse_allreduce_algo ", ccl_coll_algorithm_to_str(algo));
            return ccl_status_invalid_arguments;
    }

    return status;
}

ccl_request* ccl_allgatherv_impl(const void* send_buf,
                                 size_t send_count,
                                 void* recv_buf,
                                 const size_t* recv_counts,
                                 ccl_datatype_t dtype,
                                 const ccl_coll_attr_t* attr,
                                 ccl_comm* comm,
                                 const ccl_stream* stream)
{
    ccl_coll_param coll_param{};
    coll_param.ctype = ccl_coll_allgatherv;
    coll_param.send_buf = send_buf;
    coll_param.recv_buf = recv_buf;
    coll_param.send_count = send_count;
    coll_param.recv_counts = recv_counts;
    coll_param.dtype = ccl_datatype_get(dtype);
    coll_param.stream = stream;
    coll_param.comm = comm;

    ccl_sched_key key{};
    key.ctype = ccl_coll_allgatherv;
    key.buf = (void*)recv_counts;
    key.count1 = send_count;
    key.dtype = dtype;
    key.comm = comm;

    auto req = ccl_coll_create(coll_param, attr, std::move(key));
    LOG_DEBUG("coll ", ccl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}

ccl_request* ccl_allreduce_impl(const void* send_buf,
                                void* recv_buf,
                                size_t count,
                                ccl_datatype_t dtype,
                                ccl_reduction_t reduction,
                                const ccl_coll_attr_t* attr,
                                ccl_comm* comm,
                                const ccl_stream* stream)
{
    ccl_coll_param coll_param{};
    coll_param.ctype = ccl_coll_allreduce;
    coll_param.send_buf = send_buf;
    coll_param.recv_buf = recv_buf;
    coll_param.count = count;
    coll_param.dtype = ccl_datatype_get(dtype);
    coll_param.reduction = reduction;
    coll_param.stream = stream;
    coll_param.comm = comm;

    ccl_sched_key key{};
    key.ctype = ccl_coll_allreduce;
    key.count1 = count;
    key.dtype = dtype;
    key.reduction = reduction;
    key.comm = comm;

    auto req = ccl_coll_create(coll_param, attr, std::move(key));
    LOG_DEBUG("coll ", ccl_coll_type_to_str(coll_param.ctype), " created, req ", req, " count ", count);
    return req;
}

void ccl_barrier_impl(ccl_comm* comm, const ccl_stream* stream)
{
    ccl_coll_param coll_param{};
    coll_param.ctype = ccl_coll_barrier;
    coll_param.dtype = ccl_dtype_internal_char;
    coll_param.stream = stream;
    coll_param.comm = comm;

    ccl_coll_attr_t attributes{};
    attributes.synchronous = 1;

    ccl_sched_key key{};
    key.ctype = ccl_coll_barrier;
    key.comm = comm;

    ccl_coll_create(coll_param, &attributes, std::move(key));
}

ccl_request* ccl_bcast_impl(void* buf,
                            size_t count,
                            ccl_datatype_t dtype,
                            size_t root,
                            const ccl_coll_attr_t* attr,
                            ccl_comm* comm,
                            const ccl_stream* stream)
{
    ccl_coll_param coll_param{};
    coll_param.ctype = ccl_coll_bcast;
    coll_param.buf = buf;
    coll_param.count = count;
    coll_param.dtype = ccl_datatype_get(dtype);
    coll_param.root = root;
    coll_param.stream = stream;
    coll_param.comm = comm;

    ccl_sched_key key{};
    key.ctype = ccl_coll_bcast;
    key.count1 = count;
    key.dtype = dtype;
    key.root = root;
    key.comm = comm;

    auto req = ccl_coll_create(coll_param, attr, std::move(key));
    LOG_DEBUG("coll ", ccl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}

ccl_request* ccl_reduce_impl(const void* send_buf,
                             void* recv_buf,
                             size_t count,
                             ccl_datatype_t dtype,
                             ccl_reduction_t reduction,
                             size_t root,
                             const ccl_coll_attr_t* attr,
                             ccl_comm* comm,
                             const ccl_stream* stream)
{
    ccl_coll_param coll_param{};
    coll_param.ctype = ccl_coll_reduce;
    coll_param.send_buf = send_buf;
    coll_param.recv_buf = recv_buf;
    coll_param.count = count;
    coll_param.dtype = ccl_datatype_get(dtype);
    coll_param.reduction = reduction;
    coll_param.root = root;
    coll_param.stream = stream;
    coll_param.comm = comm;

    ccl_sched_key key{};
    key.ctype = ccl_coll_reduce;
    key.count1 = count;
    key.dtype = dtype;
    key.reduction = reduction;
    key.root = root;
    key.comm = comm;

    auto req = ccl_coll_create(coll_param, attr, std::move(key));
    LOG_DEBUG("coll ", ccl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}

ccl_request* ccl_sparse_allreduce_impl(const void* send_ind_buf, size_t send_ind_count,
                                       const void* send_val_buf, size_t send_val_count,
                                       void** recv_ind_buf, size_t* recv_ind_count,
                                       void** recv_val_buf, size_t* recv_val_count,
                                       ccl_datatype_t index_dtype, ccl_datatype_t dtype,
                                       ccl_reduction_t reduction, const ccl_coll_attr_t* attr,
                                       ccl_comm* comm, const ccl_stream* stream)
{
    ccl_coll_param coll_param{};
    coll_param.ctype = ccl_coll_sparse_allreduce;
    coll_param.sparse_param.send_ind_buf = send_ind_buf;
    coll_param.sparse_param.send_ind_count = send_ind_count;
    coll_param.sparse_param.send_val_buf = send_val_buf;
    coll_param.sparse_param.send_val_count = send_val_count;
    coll_param.sparse_param.recv_ind_buf = recv_ind_buf;
    coll_param.sparse_param.recv_ind_count = recv_ind_count;
    coll_param.sparse_param.recv_val_buf = recv_val_buf;
    coll_param.sparse_param.recv_val_count = recv_val_count;
    coll_param.dtype = ccl_datatype_get(dtype);
    coll_param.sparse_param.itype = ccl_datatype_get(index_dtype);
    coll_param.reduction = reduction;
    coll_param.stream = stream;
    coll_param.comm = comm;

    ccl_sched_key key{};
    key.ctype = ccl_coll_sparse_allreduce;
    key.count1 = send_ind_count;
    key.count2 = send_val_count;
    key.count3 = recv_ind_count;
    key.count4 = recv_val_count;
    key.itype = index_dtype;
    key.dtype = dtype;
    key.reduction = reduction;
    key.comm = comm;

    auto req = ccl_coll_create(coll_param, attr, std::move(key));
    LOG_DEBUG("coll ", ccl_coll_type_to_str(coll_param.ctype), " created, req ", req);
    return req;
}
