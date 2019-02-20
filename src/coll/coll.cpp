#include <mlsl.hpp>
#include "coll/coll.hpp"
#include "coll/coll_algorithms.hpp"


mlsl_coll_attr_t* default_coll_attr = NULL;

mlsl_status_t mlsl_coll_create_attr(mlsl_coll_attr_t** coll_attr)
{
    mlsl_coll_attr_t* attr = static_cast<mlsl_coll_attr_t*>(MLSL_CALLOC(sizeof(mlsl_coll_attr_t), "coll_attr"));
    *coll_attr = attr;
    return mlsl_status_success;
}

mlsl_status_t mlsl_coll_free_attr(mlsl_coll_attr_t* coll_attr)
{
    MLSL_FREE(coll_attr);
    return mlsl_status_success;
}

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
        case mlsl_coll_custom:
            return "CUSTOM";
        case mlsl_coll_service_temporal:
            return "SERVICE_TEMP";
        case mlsl_coll_service_persistent:
            return "SERVICE_PERS";
        default:
            MLSL_ASSERT_FMT(0, "unexpected coll_type %d", type);
            return "(out of range)";
    }
}

static mlsl_status_t mlsl_coll_create(mlsl_request** req,
                                      const mlsl_coll_attr_t* attributes,
                                      mlsl_sched_cache_key& key,
                                      mlsl_sched_coll_param& sched_param)
{
    mlsl_sched* sched = nullptr;
    mlsl_sched_cache_entry* entry = nullptr;
    mlsl_comm* tensor_comm = nullptr;
    bool sched_to_be_posponed = false;
    const mlsl_coll_attr_t* attr = attributes ? attributes : global_data.default_coll_attr;

    MLSL_ASSERTP((sched_param.ctype == mlsl_coll_allreduce) ||
                 !(attr->prologue_fn || attr->epilogue_fn || attr->reduction_fn));

    if (attr->match_id && env_data.out_of_order_support)
    {
        if (strlen(attr->match_id) > MLSL_MATCH_ID_MAX_LEN)
        {
            MLSL_LOG(ERROR, "Match_id length exceeds limit %d", MLSL_MATCH_ID_MAX_LEN);
            return mlsl_status_invalid_arguments;
        }
        //todo: need to sync checking of tensor communicator and possible creation of tensor
        tensor_comm = global_data.ooo_handler->get_comm_for_tensor(attr->match_id);
        if (!tensor_comm)
        {
            sched_to_be_posponed = true;
        }
    }
    if (attr->to_cache)
    {
        key.prologue_fn = attr->prologue_fn;
        key.epilogue_fn = attr->epilogue_fn;
        key.reduction_fn = attr->reduction_fn;
        key.priority = attr->priority;
        key.synchronous = attr->synchronous;

        if (attr->match_id)
        {
            strncpy(key.match_id, attr->match_id, MLSL_MATCH_ID_MAX_LEN - 1);
        }

        mlsl_sched_cache_get_entry(global_data.sched_cache, &key, &entry);
        sched = entry->sched;
    }
    if (!sched)
    {
        sched = new mlsl_sched(sched_param);
        sched->set_coll_attr(attr);
        MLSL_LOG(DEBUG, "didn't find sched, create new one %p, type %s", sched,
                 mlsl_coll_type_to_str(sched->coll_param.ctype));
        if (tensor_comm)
        {
            sched->coll_param.comm = tensor_comm;
        }

        sched->commit(global_data.parallelizer);

        if (entry)
        {
            entry->sched = sched;
        }
    }
    else
    {
        MLSL_LOG(DEBUG, "found sched, reuse %p, type %s", sched, mlsl_coll_type_to_str(sched->coll_param.ctype));
        if (tensor_comm)
        {
            sched->coll_param.comm = tensor_comm;
        }
        sched->set_coll_attr(attr);
    }
    if (!sched_to_be_posponed)
    {
        *req = sched->start(global_data.executor.get());
        if (attr->synchronous)
        {
            mlsl_wait(*req);
            *req = nullptr;
        }
    }
    else
    {
        MLSL_LOG(INFO, "Sched %p postponed for tensor comm resolution", sched);
        std::string tensor_name{attr->match_id};
        /* sched->coll_param.comm points to the user defined or global comm */
        if (sched->coll_param.comm->rank() == 0 &&
            !global_data.ooo_handler->is_bcast_in_progress(tensor_name))
        {
            MLSL_LOG(INFO, "Root rank broadcasts tensor %s", attr->match_id);
            mlsl_sched* service_sched = global_data.ooo_handler->build_bcast_sched(
                tensor_name.c_str());
            global_data.executor->start_sched(service_sched);
        }
        /* root rank already broadcasts the tensor or it is not root rank */
        global_data.ooo_handler->postpone_for_tensor(attr->match_id, sched);
        *req = sched->reset_request();
    }

    return mlsl_status_success;
}

mlsl_status_t mlsl_coll_build_barrier(mlsl_sched* sched)
{
    mlsl_status_t status;
    sched->coll_param.ctype = mlsl_coll_barrier;
    MLSL_CALL(mlsl_coll_build_dissemination_barrier(sched));
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
    MLSL_CALL(mlsl_coll_build_scatter_ring_allgather_bcast(sched, buf, count, dtype, root));
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

    if (count < sched->coll_param.comm->pof2())
        MLSL_CALL(mlsl_coll_build_binomial_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));
    else
        MLSL_CALL(mlsl_coll_build_rabenseifner_reduce(sched, send_buf, recv_buf, count, dtype, reduction, root));

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
                    MLSL_ASSERTP_FMT(0, "unexpected allreduce_algo '%s'",
                                     mlsl_allreduce_algo_to_str(env_data.allreduce_algo));
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
            default:
                MLSL_ASSERTP_FMT(0, "unexpected allreduce_algo '%s'",
                                 mlsl_allreduce_algo_to_str(env_data.allreduce_algo));
                return mlsl_status_invalid_arguments;
        }
    }

    return status;
}

mlsl_status_t mlsl_bcast(
    void* buf,
    size_t count,
    mlsl_datatype_t dtype,
    size_t root,
    const mlsl_coll_attr_t* attributes,
    mlsl_comm_t communicator,
    mlsl_request** req)
{
    try
    {
        auto comm = static_cast<mlsl_comm*>(communicator);

        mlsl_sched_coll_param sched_param{};
        sched_param.ctype = mlsl_coll_bcast;
        sched_param.buf = buf;
        sched_param.count = count;
        sched_param.dtype = mlsl_datatype_get(dtype);
        sched_param.root = root;
        sched_param.comm = comm ? comm : global_data.comm.get();

        mlsl_sched_cache_key key{};
        key.ctype = mlsl_coll_bcast;
        key.buf1 = buf;
        key.count1 = count;
        key.dtype = dtype;
        key.root = root;
        key.comm = comm;

        return mlsl_coll_create(req, attributes, key, sched_param);
    }
    COMMON_CATCH_BLOCK();
}

mlsl_status_t mlsl_reduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    mlsl_datatype_t dtype,
    mlsl_reduction_t reduction,
    size_t root,
    const mlsl_coll_attr_t* attributes,
    mlsl_comm_t communicator,
    mlsl_request** req)
{
    try
    {
        auto comm = static_cast<mlsl_comm*>(communicator);

        mlsl_sched_coll_param sched_param{};
        sched_param.ctype = mlsl_coll_reduce;
        sched_param.send_buf = send_buf;
        sched_param.recv_buf = recv_buf;
        sched_param.count = count;
        sched_param.dtype = mlsl_datatype_get(dtype);
        sched_param.reduction = reduction;
        sched_param.root = root;
        sched_param.comm = comm ? comm : global_data.comm.get();

        mlsl_sched_cache_key key{};
        key.ctype = mlsl_coll_reduce;
        key.buf1 = (void*) send_buf;
        key.buf2 = recv_buf;
        key.count1 = count;
        key.dtype = dtype;
        key.reduction = reduction;
        key.root = root;
        key.comm = comm;

        return mlsl_coll_create(req, attributes, key, sched_param);
    }
    COMMON_CATCH_BLOCK();
}

mlsl_status_t mlsl_allreduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    mlsl_datatype_t dtype,
    mlsl_reduction_t reduction,
    const mlsl_coll_attr_t* attributes,
    mlsl_comm_t communicator,
    mlsl_request** req)
{
    try
    {
        auto comm = static_cast<mlsl_comm*>(communicator);

        mlsl_sched_coll_param sched_param{};
        sched_param.ctype = mlsl_coll_allreduce;
        sched_param.send_buf = send_buf;
        sched_param.recv_buf = recv_buf;
        sched_param.count = count;
        sched_param.dtype = mlsl_datatype_get(dtype);
        sched_param.reduction = reduction;
        sched_param.comm = comm ? comm : global_data.comm.get();

        mlsl_sched_cache_key key{};
        key.ctype = mlsl_coll_reduce;
        key.buf1 = (void*) send_buf;
        key.buf2 = recv_buf;
        key.count1 = count;
        key.dtype = dtype;
        key.reduction = reduction;
        key.comm = comm;

        return mlsl_coll_create(req, attributes, key, sched_param);
    }
    COMMON_CATCH_BLOCK();
}

mlsl_status_t mlsl_allgatherv(
    const void* send_buf,
    size_t send_count,
    void* recv_buf,
    size_t* recv_counts,
    mlsl_datatype_t dtype,
    const mlsl_coll_attr_t* attributes,
    mlsl_comm_t communicator,
    mlsl_request** req)
{
    try
    {
        auto comm = static_cast<mlsl_comm*>(communicator);

        mlsl_sched_coll_param sched_param{};
        sched_param.ctype = mlsl_coll_allgatherv;
        sched_param.send_buf = send_buf;
        sched_param.recv_buf = recv_buf;
        sched_param.send_count = send_count;
        sched_param.recv_counts = recv_counts;
        sched_param.dtype = mlsl_datatype_get(dtype);
        sched_param.comm = comm ? comm : global_data.comm.get();

        mlsl_sched_cache_key key{};
        key.ctype = mlsl_coll_allgatherv;
        key.buf1 = (void*) send_buf;
        key.buf2 = recv_buf;
        key.buf3 = recv_counts;
        key.count1 = send_count;
        key.comm = comm;

        return mlsl_coll_create(req, attributes, key, sched_param);
    }
    COMMON_CATCH_BLOCK();
}

mlsl_status_t mlsl_barrier(mlsl_comm_t communicator)
{
    try
    {
        auto comm = static_cast<mlsl_comm*>(communicator);
        mlsl_request* barrier_req;
        mlsl_coll_attr_t attributes{};
        attributes.synchronous = 1;


        mlsl_sched_coll_param sched_param{};
        sched_param.ctype = mlsl_coll_barrier;
        sched_param.dtype = mlsl_dtype_internal_char;
        sched_param.comm = comm ? comm : global_data.comm.get();

        mlsl_sched_cache_key key{};
        key.ctype = mlsl_coll_barrier;
        key.comm = comm;

        return mlsl_coll_create(&barrier_req, &attributes, key, sched_param);
    }
    COMMON_CATCH_BLOCK();
}
