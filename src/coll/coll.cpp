#include <mlsl.hpp>
#include "coll/coll.hpp"
#include "coll/coll_algorithms.hpp"

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
        case mlsl_coll_sparse_allreduce:
            return "SPARSE_ALLREDUCE";
        case mlsl_coll_service_temporal:
            return "SERVICE_TEMP";
        case mlsl_coll_service_persistent:
            return "SERVICE_PERS";
        case mlsl_coll_none:
            return "NONE";
        default:
            MLSL_FATAL("unexpected coll_type %d", type);
            return "UNKNOWN";
    }
}

static mlsl_request* mlsl_coll_create(const mlsl_coll_attr_t* attributes,
                                      mlsl_sched_cache_key& key,
                                      mlsl_coll_param& coll_param)
{
    mlsl_sched* sched = nullptr;
    mlsl_sched_cache_entry* entry = nullptr;
    mlsl_comm* tensor_comm = nullptr;
    bool sched_to_be_posponed = false;
    bool should_commit = false;
    bool was_fused = false;
    mlsl_request* request = nullptr;
    const mlsl_coll_attr_t* attr = attributes ? attributes : global_data.default_coll_attr;

    MLSL_THROW_IF_NOT((coll_param.ctype == mlsl_coll_allreduce) ||
                      !(attr->prologue_fn || attr->epilogue_fn || attr->reduction_fn),
                      "Incorrect input");

    if (attr->match_id && env_data.out_of_order_support)
    {
        MLSL_THROW_IF_NOT(strlen(attr->match_id) <= MLSL_MATCH_ID_MAX_LEN,
                          "match_id length exceeds limit %d", MLSL_MATCH_ID_MAX_LEN);
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
        sched = new mlsl_sched(coll_param);
        MLSL_LOG(DEBUG, "didn't find sched, create new one %p, type %s",
                 sched, mlsl_coll_type_to_str(sched->coll_param.ctype));

        if (entry)
        {
            entry->sched = sched;
        }

        sched->set_coll_attr(attr);
        should_commit = true;
    }
    else
    {
        MLSL_LOG(DEBUG, "found sched, reuse %p, type %s",
                 sched, mlsl_coll_type_to_str(sched->coll_param.ctype));
    }

    if (tensor_comm)
    {
        sched->coll_param.comm = tensor_comm;
    }

    if (env_data.enable_fusion)
    {
        MLSL_LOG(DEBUG, "try to add sched %p, ctype %d", sched, sched->coll_param.ctype);
        was_fused = global_data.fusion_manager->add(sched);
        if (was_fused)
        {
            request = sched->req;
            return request;
        }
    }

    MLSL_ASSERT(!was_fused, "");

    if (should_commit)
    {
        sched->commit(global_data.parallelizer.get());
    }

    if (!sched_to_be_posponed)
    {
        request = sched->start(global_data.executor.get());
        if (attr->synchronous)
        {
            mlsl_wait(request);
            request = nullptr;
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
        request = sched->reset_request();
    }

    return request;
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
                    MLSL_FATAL("unexpected allreduce_algo '%s'",
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
                MLSL_FATAL("unexpected allreduce_algo '%s'",
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
            MLSL_FATAL("unexpected sparse_allreduce_algo %d", env_data.sparse_allreduce_algo);
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

    mlsl_sched_cache_key key{};
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
    MLSL_LOG(INFO, "coll %s created, req %p", mlsl_coll_type_to_str(coll_param.ctype), req);
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

    mlsl_sched_cache_key key{};
    key.ctype = mlsl_coll_bcast;
    key.buf1 = buf;
    key.count1 = count;
    key.dtype = dtype;
    key.root = root;
    key.comm = communicator;

    auto req = mlsl_coll_create(attributes, key, coll_param);
    MLSL_LOG(INFO, "coll %s created, req %p", mlsl_coll_type_to_str(coll_param.ctype), req);
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

    mlsl_sched_cache_key key{};
    key.ctype = mlsl_coll_reduce;
    key.buf1 = (void*) send_buf;
    key.buf2 = recv_buf;
    key.count1 = count;
    key.dtype = dtype;
    key.reduction = reduction;
    key.root = root;
    key.comm = communicator;

    auto req = mlsl_coll_create(attributes, key, coll_param);
    MLSL_LOG(INFO, "coll %s created, req %p", mlsl_coll_type_to_str(coll_param.ctype), req);
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

    mlsl_sched_cache_key key{};
    key.ctype = mlsl_coll_reduce;
    key.buf1 = (void*) send_buf;
    key.buf2 = recv_buf;
    key.count1 = count;
    key.dtype = dtype;
    key.reduction = reduction;
    key.comm = communicator;

    auto req = mlsl_coll_create(attributes, key, coll_param);
    MLSL_LOG(INFO, "coll %s created, req %p", mlsl_coll_type_to_str(coll_param.ctype), req);
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

    mlsl_sched_cache_key key{};
    key.ctype = mlsl_coll_allgatherv;
    key.buf1 = (void*) send_buf;
    key.buf2 = recv_buf;
    key.buf3 = recv_counts;
    key.count1 = send_count;
    key.comm = communicator;

    auto req = mlsl_coll_create(attributes, key, coll_param);
    MLSL_LOG(INFO, "coll %s created, req %p", mlsl_coll_type_to_str(coll_param.ctype), req);
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

    mlsl_sched_cache_key key{};
    key.ctype = mlsl_coll_barrier;
    key.comm = communicator;

    mlsl_coll_create(&attributes, key, coll_param);
}
