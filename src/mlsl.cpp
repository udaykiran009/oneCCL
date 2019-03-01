#include "exec/exec.hpp"
#include "common/global/global.hpp"
#include "sched/sched_queue.hpp"

mlsl_status_t mlsl_init()
{
    try
    {
        mlsl_env_parse();
        mlsl_env_print();
        mlsl_datatype_init();

        global_data.sched_cache = std::unique_ptr<mlsl_sched_cache>(new mlsl_sched_cache());
        global_data.parallelizer = std::unique_ptr<mlsl_parallelizer>(new mlsl_parallelizer(env_data.worker_count));

        if (env_data.enable_fusion)
        {
            global_data.fusion_manager =
                std::unique_ptr<mlsl_fusion_manager>(new mlsl_fusion_manager());
        }

        global_data.executor = std::unique_ptr<mlsl_executor>(new mlsl_executor(env_data.worker_count,
                                                                                env_data.worker_affinity,
                                                                                env_data.out_of_order_support != 0 ||
                                                                                env_data.enable_fusion != 0));

        global_data.comm_ids = std::unique_ptr<comm_id_storage>(new comm_id_storage(mlsl_comm::max_comm_count));

        global_data.comm = std::make_shared<mlsl_comm>(global_data.executor->proc_idx,
                                                       global_data.executor->proc_count,
                                                       std::unique_ptr<comm_id>(new comm_id(*global_data.comm_ids)));

        mlsl_coll_create_attr(&global_data.default_coll_attr);
        global_data.default_coll_attr->to_cache = 1;

        if (env_data.out_of_order_support)
        {
            global_data.ooo_handler =
                std::unique_ptr<out_of_order::ooo_match>(
                    new out_of_order::ooo_match(*global_data.executor, *global_data.comm_ids));
        }

        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t mlsl_finalize()
{
    try
    {
        global_data.sched_cache.reset();
        global_data.ooo_handler.reset();
        global_data.executor.reset();
        global_data.fusion_manager.reset();
        global_data.comm.reset();
        global_data.comm_ids.reset();
        global_data.parallelizer.reset();

        if (global_data.default_coll_attr)
        {
            mlsl_coll_free_attr(global_data.default_coll_attr);
        }

        mlsl_env_free();
        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t MLSL_API mlsl_wait(mlsl_request_t req)
{
    try
    {
        if (!req)
        {
            MLSL_LOG(ERROR, "empty request");
            return mlsl_status_success;
        }

        auto request = static_cast<mlsl_request*>(req);

        mlsl_wait_impl(global_data.executor.get(), request);

        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t MLSL_API mlsl_test(mlsl_request_t req,
                                 int* is_completed)
{
    try
    {
        if (!req)
        {
            MLSL_LOG(ERROR, "empty request");
            if (is_completed)
            { *is_completed = 1; }
            return mlsl_status_success;
        }

        auto request = static_cast<mlsl_request*>(req);

        auto completed = mlsl_test_impl(global_data.executor.get(), request);

        *is_completed = static_cast<int>(completed);

        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t mlsl_comm_create(mlsl_comm_t* comm_t,
                               mlsl_comm_attr_t* comm_attr)
{
    MLSL_ASSERT(comm_t);
    try
    {
        mlsl_comm* comm = nullptr;
        if (!comm_attr)
        {
            MLSL_LOG(DEBUG, "Duplicating global comm");
            comm = new mlsl_comm(global_data.comm->rank(), global_data.comm->size(),
                                 std::unique_ptr<comm_id>(new comm_id(*global_data.comm_ids)));
        }
        else
        {

            comm = mlsl_comm::create_with_color(comm_attr->color, global_data.comm_ids.get(), global_data.comm.get());
        }

        *comm_t = static_cast<void*>(comm);
        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t mlsl_comm_free(mlsl_comm_t comm_t)
{
    MLSL_ASSERT(comm_t);
    MLSL_LOG(DEBUG, "Free comm %p", comm_t);
    try
    {
        auto comm = static_cast<mlsl_comm*>(comm_t);
        delete comm;
        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t MLSL_API mlsl_get_comm_rank(mlsl_comm_t comm_t,
                                          size_t* out_rank)
{
    try
    {
        auto comm = static_cast<mlsl_comm*>(comm_t);
        auto rank = comm ? comm->rank() : global_data.comm->rank();
        if (out_rank)
        {
            *out_rank = rank;
        }
        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t MLSL_API mlsl_get_comm_size(mlsl_comm_t comm_t,
                                          size_t* out_size)
{
    try
    {
        auto comm = static_cast<mlsl_comm*>(comm_t);
        auto size = comm ? comm->size() : global_data.comm->size();
        if (out_size)
        {
            *out_size = size;
        }
        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t MLSL_API mlsl_bcast(
    void* buf,
    size_t count,
    mlsl_datatype_t dtype,
    size_t root,
    const mlsl_coll_attr_t* attributes,
    mlsl_comm_t communicator,
    mlsl_request_t* req)
{
    try
    {
        if (!req)
        { return mlsl_status_invalid_arguments; }

        auto comm = static_cast<mlsl_comm*>(communicator);
        mlsl_comm* real_comm = comm ?: global_data.comm.get();

        auto request = mlsl_bcast_impl(buf, count, dtype, root, attributes, real_comm);
        *req = static_cast<mlsl_request_t>(request);

        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

mlsl_status_t MLSL_API mlsl_reduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    mlsl_datatype_t dtype,
    mlsl_reduction_t reduction,
    size_t root,
    const mlsl_coll_attr_t* attributes,
    mlsl_comm_t communicator,
    mlsl_request_t* req)
{
    try
    {
        if (!req)
        { return mlsl_status_invalid_arguments; }

        auto comm = static_cast<mlsl_comm*>(communicator);
        mlsl_comm* real_comm = comm ?: global_data.comm.get();

        auto request = mlsl_reduce_impl(send_buf, recv_buf, count, dtype, reduction, root, attributes, real_comm);
        *req = static_cast<mlsl_request_t>(request);

        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

mlsl_status_t MLSL_API mlsl_allreduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    mlsl_datatype_t dtype,
    mlsl_reduction_t reduction,
    const mlsl_coll_attr_t* attributes,
    mlsl_comm_t communicator,
    mlsl_request_t* req)
{
    try
    {
        if (!req)
        { return mlsl_status_invalid_arguments; }

        auto comm = static_cast<mlsl_comm*>(communicator);
        mlsl_comm* real_comm = comm ?: global_data.comm.get();

        auto request = mlsl_allreduce_impl(send_buf, recv_buf, count, dtype, reduction, attributes, real_comm);
        *req = static_cast<mlsl_request_t>(request);

        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

mlsl_status_t MLSL_API mlsl_allgatherv(
    const void* send_buf,
    size_t send_count,
    void* recv_buf,
    size_t* recv_counts,
    mlsl_datatype_t dtype,
    const mlsl_coll_attr_t* attributes,
    mlsl_comm_t communicator,
    mlsl_request_t* req)
{
    try
    {
        if (!req)
        { return mlsl_status_invalid_arguments; }

        auto comm = static_cast<mlsl_comm*>(communicator);
        mlsl_comm* real_comm = comm ?: global_data.comm.get();

        auto request = mlsl_allgatherv_impl(send_buf, send_count, recv_buf, recv_counts, dtype, attributes, real_comm);
        *req = static_cast<mlsl_request_t>(request);

        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

mlsl_status_t MLSL_API mlsl_sparse_allreduce(const void* send_ind_buf,
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
                                             mlsl_comm_t communicator,
                                             mlsl_request_t* req)
{
    try
    {
        if (!req)
        { return mlsl_status_invalid_arguments; }

        auto comm = static_cast<mlsl_comm*>(communicator);
        mlsl_comm* real_comm = comm ?: global_data.comm.get();

        auto request = mlsl_sparse_allreduce_impl(send_ind_buf, send_ind_count, send_val_buf, send_val_count,
                                                  recv_ind_buf, recv_ind_count, recv_val_buf,
                                                  recv_val_count, index_dtype, dtype, reduction, attributes, real_comm);
        *req = static_cast<mlsl_request_t>(request);

        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

mlsl_status_t MLSL_API mlsl_barrier(mlsl_comm_t communicator)
{
    try
    {
        auto comm = static_cast<mlsl_comm*>(communicator);
        mlsl_comm* real_comm = comm ?: global_data.comm.get();

        mlsl_barrier_impl(real_comm);

        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK();
}
