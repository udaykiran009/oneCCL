#include "ccl.hpp"
#include "coll/algorithms/algorithms.hpp"
#include "coll/algorithms/allreduce/allreduce_2d.hpp"
#include "coll/algorithms/sparse.hpp"
#include "coll/coll.hpp"
#include "coll/selection/selection.hpp"
#include "common/global/global.hpp"
#include "common/request/request.hpp"
#include "exec/exec.hpp"
#include "fusion/fusion.hpp"
#include "unordered_coll/unordered_coll.hpp"

ccl_coll_attr::ccl_coll_attr(const ccl_coll_attr_t* attr)
{
    *this = attr ?: global_data.default_coll_attr.get();
}

ccl_coll_attr& ccl_coll_attr::operator= (const ccl_coll_attr_t* attr)
{
    prologue_fn = attr->prologue_fn;
    epilogue_fn = attr->epilogue_fn;
    reduction_fn = attr->reduction_fn;
    priority = attr->priority;
    synchronous = attr->synchronous;
    to_cache = attr->to_cache && attr->match_id && attr->match_id[0];
    vector_buf = attr->vector_buf;
    match_id = (attr->match_id ? attr->match_id : "");

    if (to_cache != attr->to_cache)
        LOG_INFO("collective caching is requested but no match_id is provided, disable caching");

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
        case ccl_coll_alltoall:
            return "alltoall";
        case ccl_coll_alltoallv:
            return "alltoallv";
        case ccl_coll_barrier:
            return "barrier";
        case ccl_coll_bcast:
            return "bcast";
        case ccl_coll_reduce:
            return "reduce";
        case ccl_coll_reduce_scatter:
            return "reduce_scatter";
        case ccl_coll_sparse_allreduce:
            return "sparse_allreduce";
        case ccl_coll_internal:
            return "internal";
        default:
            CCL_FATAL("unexpected coll_type ", type);
            return "unknown";
    }
}

/* param is not const because param.comm can be updated for unordered colls */
static ccl_request* ccl_coll_create(ccl_coll_param& param,
                                    const ccl_coll_attr& attr)
{
    /* 1. decide whether schedule should be postponed (this includes caching and staring) */
    bool postpone_schedule = false;
    if (env_data.enable_unordered_coll)
    {
        if (!attr.match_id.empty())
        {
            auto comm = global_data.unordered_coll_manager->get_comm(std::string(attr.match_id)).get();
            if (!comm)
            {
                if (attr.synchronous)
                {
                    CCL_THROW("unsupported collective (synchronous && unordered && !communicator)");
                }
                LOG_DEBUG("didn't find comm for match_id ", attr.match_id, ", postpone schedule");
                postpone_schedule = true;
            }
            else
            {
                LOG_DEBUG("found comm ", comm->id(), " for match_id ", attr.match_id);
                param.comm = comm;
            }
        }
        else
        {
            /* use comm provided by user, it is ordered collective */
        }
    }

    /* 2. create or get schedule */
    ccl_master_sched* sched = ccl_master_sched::create(param, attr);

    /* 3. fuse schedule */
    if (!postpone_schedule && env_data.enable_fusion)
    {
        if (global_data.fusion_manager->add(sched))
        {
            LOG_DEBUG("sched ", sched, ", ctype ",
                      ccl_coll_type_to_str(sched->coll_param.ctype), " will be fused");
            return sched;
        }
    }

    /* 4. parallelize schedule */
    sched->commit(global_data.parallelizer.get());

    /* 5. postpone unordered coll schedule */
    if (postpone_schedule)
    {
        /* 
            user has provided match_id that has not been resolved yet.
            schedule will be postponed until comm resolution
        */
        return global_data.unordered_coll_manager->postpone(sched);
    }

    /* 6. regular schedule execution */
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
    ccl_datatype_internal_t dtype,
    ccl_comm* comm)
{
    ccl_status_t status = ccl_status_success;

    ccl_selector_param param;
    param.ctype = ccl_coll_allgatherv;
    param.recv_counts = recv_counts;
    param.dtype = dtype;
    param.comm = comm;
    param.vector_buf = sched->coll_attr.vector_buf;

    auto algo = global_data.algorithm_selector->get<ccl_coll_allgatherv>(param);

    switch (algo)
    {
        case ccl_coll_allgatherv_direct:
            CCL_CALL(ccl_coll_build_direct_allgatherv(sched, send_buf, send_count, recv_buf, recv_counts,
                                                      dtype, comm));
            break;
        case ccl_coll_allgatherv_naive:
            CCL_CALL(ccl_coll_build_naive_allgatherv(sched, send_buf, send_count, recv_buf, recv_counts,
                                                     dtype, comm));
            break;
        case ccl_coll_allgatherv_ring:
            CCL_CALL(ccl_coll_build_ring_allgatherv(sched, send_buf, send_count, recv_buf, recv_counts,
                                                    dtype, comm));
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
    ccl_reduction_t reduction,
    ccl_comm* comm)
{
    ccl_status_t status = ccl_status_success;

    ccl_selector_param param;
    param.ctype = ccl_coll_allreduce;
    param.count = count;
    param.dtype = dtype;
    param.comm = comm;

    auto algo = global_data.algorithm_selector->get<ccl_coll_allreduce>(param);

    switch (algo)
    {
        case ccl_coll_allreduce_direct:
            CCL_CALL(ccl_coll_build_direct_allreduce(sched, send_buf, recv_buf, count,
                                                     dtype, reduction, comm));
            break;
        case ccl_coll_allreduce_rabenseifner:
            CCL_CALL(ccl_coll_build_rabenseifner_allreduce(sched, send_buf, recv_buf, count,
                                                           dtype, reduction, comm));
            break;
        case ccl_coll_allreduce_starlike:
            CCL_CALL(ccl_coll_build_starlike_allreduce(sched, send_buf, recv_buf, count,
                                                       dtype, reduction, comm));
            break;
        case ccl_coll_allreduce_ring:
            CCL_CALL(ccl_coll_build_ring_allreduce(sched, send_buf, recv_buf, count,
                                                   dtype, reduction, comm));
            break;
        case ccl_coll_allreduce_ring_rma:
            CCL_CALL(ccl_coll_build_ring_rma_allreduce(sched, send_buf, recv_buf, count,
                                                       dtype, reduction, comm));
            break;
        case ccl_coll_allreduce_double_tree:
            CCL_CALL(
                ccl_coll_build_double_tree_op(sched, ccl_coll_allreduce, send_buf, recv_buf,
                                              count, dtype, reduction, comm->dtree(), comm));
            break;
        case ccl_coll_allreduce_recursive_doubling:
            CCL_CALL(ccl_coll_build_recursive_doubling_allreduce(sched, send_buf, recv_buf, count,
                                                                 dtype, reduction, comm));
            break;
        case ccl_coll_allreduce_2d:
            CCL_CALL(global_data.allreduce_2d_builder->build(sched, send_buf, recv_buf, count,
                                                             dtype, reduction, comm));
            break;
        default:
            CCL_FATAL("unexpected allreduce_algo ", ccl_coll_algorithm_to_str(algo));
            return ccl_status_invalid_arguments;
    }

    return status;
}

ccl_status_t ccl_coll_build_alltoall(
    ccl_sched* sched,
    ccl_buffer send_buf,
    ccl_buffer recv_buf,
    size_t count,
    ccl_datatype_internal_t dtype,
    ccl_comm* comm)
{
    ccl_status_t status = ccl_status_success;

    ccl_selector_param param;
    param.ctype = ccl_coll_alltoall;
    param.count = count;
    param.dtype = dtype;
    param.comm = comm;

    auto algo = global_data.algorithm_selector->get<ccl_coll_alltoall>(param);

    switch (algo)
    {
        case ccl_coll_alltoall_direct:
            CCL_CALL(ccl_coll_build_direct_alltoall(sched, send_buf, recv_buf,
                                                    count, dtype, comm));
            break;
        default:
            CCL_FATAL("unexpected alltoall_algo ", ccl_coll_algorithm_to_str(algo));
            return ccl_status_invalid_arguments;
    }

    return status;
}

ccl_status_t ccl_coll_build_alltoallv(
    ccl_sched* sched,
    ccl_buffer send_buf,
    const size_t* send_counts,
    ccl_buffer recv_buf,
    const size_t* recv_counts,
    ccl_datatype_internal_t dtype,
    ccl_comm* comm)
{
    ccl_status_t status = ccl_status_success;

    ccl_selector_param param;
    param.ctype = ccl_coll_alltoallv;
    param.dtype = dtype;
    param.comm = comm;

    auto algo = global_data.algorithm_selector->get<ccl_coll_alltoallv>(param);

    switch (algo)
    {
        case ccl_coll_alltoallv_direct:
            CCL_CALL(ccl_coll_build_direct_alltoallv(sched, send_buf, send_counts,
                                                     recv_buf, recv_counts, dtype, comm));
            break;
        default:
            CCL_FATAL("unexpected alltoallv_algo ", ccl_coll_algorithm_to_str(algo));
            return ccl_status_invalid_arguments;
    }

    return status;
}

ccl_status_t ccl_coll_build_barrier(ccl_sched* sched, ccl_comm* comm)
{
    ccl_status_t status = ccl_status_success;

    ccl_selector_param param;
    param.ctype = ccl_coll_barrier;
    param.count = 0;
    param.dtype = ccl_dtype_internal_char;
    param.comm = comm;

    auto algo = global_data.algorithm_selector->get<ccl_coll_barrier>(param);

    switch (algo)
    {
        case ccl_coll_barrier_direct:
            CCL_CALL(ccl_coll_build_direct_barrier(sched, comm));
            break;
        case ccl_coll_barrier_ring:
            CCL_CALL(ccl_coll_build_dissemination_barrier(sched, comm));
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
                                  size_t root,
                                  ccl_comm* comm)
{
    ccl_status_t status = ccl_status_success;

    ccl_selector_param param;
    param.ctype = ccl_coll_bcast;
    param.count = count;
    param.dtype = dtype;
    param.comm = comm;

    auto algo = global_data.algorithm_selector->get<ccl_coll_bcast>(param);

    switch (algo)
    {
        case ccl_coll_bcast_direct:
            CCL_CALL(ccl_coll_build_direct_bcast(sched, buf, count, dtype, root, comm));
            break;
        case ccl_coll_bcast_ring:
            CCL_CALL(ccl_coll_build_scatter_ring_allgather_bcast(sched, buf, count, dtype, root, comm));
            break;
        case ccl_coll_bcast_double_tree:
            CCL_CALL(ccl_coll_build_double_tree_op(sched, ccl_coll_bcast, ccl_buffer(), buf, count, dtype,
                                                   ccl_reduction_custom,
                                                   root == 0 ? comm->dtree() :
                                                   comm->dtree().copy_with_new_root(root), comm));
            break;
        case ccl_coll_bcast_naive:
            CCL_CALL(ccl_coll_build_naive_bcast(sched, buf, count, dtype, root, comm));
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
                                   size_t root,
                                   ccl_comm* comm)
{
    ccl_status_t status = ccl_status_success;

    ccl_selector_param param;
    param.ctype = ccl_coll_reduce;
    param.count = count;
    param.dtype = dtype;
    param.comm = comm;

    auto algo = global_data.algorithm_selector->get<ccl_coll_reduce>(param);

    switch (algo)
    {
        case ccl_coll_reduce_direct:
            CCL_CALL(ccl_coll_build_direct_reduce(sched, send_buf, recv_buf, count,
                                                  dtype, reduction, root, comm));
            break;
        case ccl_coll_reduce_rabenseifner:
            CCL_CALL(ccl_coll_build_rabenseifner_reduce(sched, send_buf, recv_buf, count,
                                                        dtype, reduction, root, comm));
            break;
        case ccl_coll_reduce_tree:
            CCL_CALL(ccl_coll_build_binomial_reduce(sched, send_buf, recv_buf, count,
                                                    dtype, reduction, root, comm));
            break;
        case ccl_coll_reduce_double_tree:
            CCL_CALL(ccl_coll_build_double_tree_op(sched, ccl_coll_reduce, send_buf, recv_buf, count, dtype,
                                                   reduction, root == 0 ? comm->dtree() :
                                                   comm->dtree().copy_with_new_root(root), comm));
            break;
        default:
            CCL_FATAL("unexpected reduce_algo ", ccl_coll_algorithm_to_str(algo));
            return ccl_status_invalid_arguments;
    }

    return status;
}

ccl_status_t ccl_coll_build_reduce_scatter(ccl_sched* sched,
                                           ccl_buffer send_buf,
                                           ccl_buffer recv_buf,
                                           size_t send_count,
                                           ccl_datatype_internal_t dtype,
                                           ccl_reduction_t reduction,
                                           ccl_comm* comm)
{
    ccl_status_t status = ccl_status_success;

    ccl_selector_param param;
    param.ctype = ccl_coll_reduce_scatter;
    param.count = send_count;
    param.dtype = dtype;
    param.comm = comm;

    auto algo = global_data.algorithm_selector->get<ccl_coll_reduce_scatter>(param);

    switch (algo)
    {
        case ccl_coll_reduce_scatter_ring:
            CCL_CALL(ccl_coll_build_ring_reduce_scatter(sched, send_buf, recv_buf, send_count,
                                                        dtype, reduction, comm));
            break;
        default:
            CCL_FATAL("unexpected reduce_scatter_algo ", ccl_coll_algorithm_to_str(algo));
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
    ccl_reduction_t reduction,
    ccl_comm* comm)
{
    ccl_status_t status = ccl_status_success;

    ccl_selector_param param;
    param.ctype = ccl_coll_sparse_allreduce;
    param.count = 0;
    param.dtype = ccl_dtype_internal_char;
    param.comm = comm;

    if (!send_ind_count || !send_val_count)
    {
        LOG_ERROR("sparse tensor indices count should be greater than zero, but got " \
                  "indices count = ", send_ind_count, ", values count = ", send_val_count);
        assert(send_ind_count && send_val_count);
        throw ccl::ccl_error(std::string(__FUNCTION__) + "sparse tensor indices count should be \
                        greater than zero, but got indices count = " + std::to_string(send_ind_count) +
                        ", values count = " + std::to_string(send_val_count));
    }
    
    if (send_ind_count > send_val_count)
    {
        CCL_FATAL("sparse collective algorithms now support only 1-D indices and \
                  multi-dimensional values format\n got indices count = ", send_ind_count,
                  ", values count = ", send_val_count);
        return ccl_status_invalid_arguments;
    }

    auto algo = global_data.algorithm_selector->get<ccl_coll_sparse_allreduce>(param);

    LOG_DEBUG("build sparse allreduce, param:",
              "\nsend_ind_buf ", send_ind_buf,
              "\nsend_ind_count ", send_ind_count,
              "\nsend_val_buf ", send_val_buf,
              "\nsend_val_count ", send_val_count,
              "\nrecv_ind_buf ", recv_ind_buf,
              "\nrecv_ind_count ", recv_ind_count,
              "\nrecv_val_buf ", recv_val_buf,
              "\nrecv_val_count ", recv_val_count,
              "\nindex_dtype ", ccl_datatype_get_name(index_dtype),
              "\nvalue_dtype ", ccl_datatype_get_name(value_dtype),
              "\nop ", ccl_reduction_to_str(reduction));

    switch (index_dtype->type)
    {
        case ccl_dtype_char:
            CCL_DEFINE_VALUE(char);
            break;
        case ccl_dtype_int:
            CCL_DEFINE_VALUE(int);
            break;
        case ccl_dtype_int64:
            CCL_DEFINE_VALUE(int64_t);
            break;
        case ccl_dtype_uint64:
            CCL_DEFINE_VALUE(uint64_t);
            break;
        default:
            CCL_FATAL("index data type ", ccl_datatype_get_name(index_dtype), " is not supported yet");
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
    ccl_coll_param param{};

    param.ctype = ccl_coll_allgatherv;
    param.send_buf = send_buf;
    param.recv_buf = recv_buf;
    param.send_count = send_count;
    param.recv_counts = recv_counts;
    param.dtype = ccl_datatype_get(dtype);
    param.stream = stream;
    param.comm = comm;

    auto req = ccl_coll_create(param, ccl_coll_attr(attr));
    LOG_DEBUG("coll ", ccl_coll_type_to_str(param.ctype), " created, req ", req);
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
    ccl_coll_param param{};

    param.ctype = ccl_coll_allreduce;
    param.send_buf = send_buf;
    param.recv_buf = recv_buf;
    param.count = count;
    param.dtype = ccl_datatype_get(dtype);
    param.reduction = reduction;
    param.stream = stream;
    param.comm = comm;

    auto req = ccl_coll_create(param, ccl_coll_attr(attr));
    LOG_DEBUG("coll ", ccl_coll_type_to_str(param.ctype), " created, req ", req, " count ", count);
    return req;
}

ccl_request* ccl_alltoall_impl(const void* send_buf,
                               void* recv_buf,
                               size_t count,
                               ccl_datatype_t dtype,
                               const ccl_coll_attr_t* attr,
                               ccl_comm* comm,
                               const ccl_stream* stream)
{
    ccl_coll_param param{};

    param.ctype = ccl_coll_alltoall;
    param.send_buf = send_buf;
    param.recv_buf = recv_buf;
    param.count = count;
    param.dtype = ccl_datatype_get(dtype);
    param.stream = stream;
    param.comm = comm;

    auto req = ccl_coll_create(param, ccl_coll_attr(attr));
    LOG_DEBUG("coll ", ccl_coll_type_to_str(param.ctype), " created, req ", req, " count ", count);
    return req;
}

ccl_request* ccl_alltoallv_impl(const void* send_buf,
                                const size_t* send_counts,
                                void* recv_buf,
                                const size_t* recv_counts,
                                ccl_datatype_t dtype,
                                const ccl_coll_attr_t* attr,
                                ccl_comm* comm,
                                const ccl_stream* stream)
{
    ccl_coll_param param{};

    param.ctype = ccl_coll_alltoallv;
    param.send_buf = send_buf;
    param.send_counts = send_counts;
    param.recv_buf = recv_buf;
    param.recv_counts = recv_counts;
    param.dtype = ccl_datatype_get(dtype);
    param.stream = stream;
    param.comm = comm;

    auto req = ccl_coll_create(param, ccl_coll_attr(attr));
    LOG_DEBUG("coll ", ccl_coll_type_to_str(param.ctype), " created, req ", req);
    return req;
}

void ccl_barrier_impl(ccl_comm* comm, const ccl_stream* stream)
{
    ccl_coll_param param{};

    param.ctype = ccl_coll_barrier;
    param.dtype = ccl_dtype_internal_char;
    param.stream = stream;
    param.comm = comm;

    ccl_coll_attr attr{};
    attr.synchronous = 1;

    ccl_coll_create(param, attr);

    if (global_data.sched_cache->try_flush())
    {
        LOG_DEBUG("flushed cache in barrier");
    }
    else
    {
        LOG_DEBUG("didn't flush cache in barrier");
    }
}

ccl_request* ccl_bcast_impl(void* buf,
                            size_t count,
                            ccl_datatype_t dtype,
                            size_t root,
                            const ccl_coll_attr_t* attr,
                            ccl_comm* comm,
                            const ccl_stream* stream)
{
    ccl_coll_param param{};

    param.ctype = ccl_coll_bcast;
    param.buf = buf;
    param.count = count;
    param.dtype = ccl_datatype_get(dtype);
    param.root = root;
    param.stream = stream;
    param.comm = comm;

    auto req = ccl_coll_create(param, ccl_coll_attr(attr));
    LOG_DEBUG("coll ", ccl_coll_type_to_str(param.ctype), " created, req ", req);
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
    ccl_coll_param param{};

    param.ctype = ccl_coll_reduce;
    param.send_buf = send_buf;
    param.recv_buf = recv_buf;
    param.count = count;
    param.dtype = ccl_datatype_get(dtype);
    param.reduction = reduction;
    param.root = root;
    param.stream = stream;
    param.comm = comm;

    auto req = ccl_coll_create(param, ccl_coll_attr(attr));
    LOG_DEBUG("coll ", ccl_coll_type_to_str(param.ctype), " created, req ", req);
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
    ccl_coll_param param{};

    param.ctype = ccl_coll_sparse_allreduce;
    param.sparse_param.send_ind_buf = send_ind_buf;
    param.sparse_param.send_ind_count = send_ind_count;
    param.sparse_param.send_val_buf = send_val_buf;
    param.sparse_param.send_val_count = send_val_count;
    param.sparse_param.recv_ind_buf = recv_ind_buf;
    param.sparse_param.recv_ind_count = recv_ind_count;
    param.sparse_param.recv_val_buf = recv_val_buf;
    param.sparse_param.recv_val_count = recv_val_count;
    param.dtype = ccl_datatype_get(dtype);
    param.sparse_param.itype = ccl_datatype_get(index_dtype);
    param.reduction = reduction;
    param.stream = stream;
    param.comm = comm;

    auto req = ccl_coll_create(param, ccl_coll_attr(attr));
    LOG_DEBUG("coll ", ccl_coll_type_to_str(param.ctype), " created, req ", req);
    return req;
}
