#include "exec/exec.hpp"
#include "out_of_order/ooo_match.hpp"
#include "fusion/fusion.hpp"
#include "parallelizer/parallelizer.hpp"
#include "sched/sched_cache.hpp"
#include "common/utils/tree.hpp"
#include "common/comm/atl_tag.hpp"

iccl_status_t iccl_init()
{
    try
    {
        iccl_env_parse();
        iccl_datatype_init();

        global_data.sched_cache = std::unique_ptr<iccl_sched_cache>(new iccl_sched_cache());
        global_data.parallelizer = std::unique_ptr<iccl_parallelizer>(new iccl_parallelizer(env_data.worker_count));

        if (env_data.enable_fusion)
        {
            global_data.fusion_manager =
                std::unique_ptr<iccl_fusion_manager>(new iccl_fusion_manager());
        }

        global_data.executor = std::unique_ptr<iccl_executor>(new iccl_executor(env_data, global_data));

        if (global_data.executor->proc_idx == 0)
        {
            iccl_env_print();
        }

        global_data.atl_tag = std::unique_ptr<iccl_atl_tag>(new iccl_atl_tag(global_data.executor->tag_bits,
                                                                             global_data.executor->max_tag));

        global_data.comm_ids = std::unique_ptr<iccl_comm_id_storage>(new iccl_comm_id_storage(iccl_comm::max_comm_count));

        global_data.comm = std::make_shared<iccl_comm>(global_data.executor->proc_idx,
                                                       global_data.executor->proc_count,
                                                       std::unique_ptr<comm_id>(
                                                           new comm_id(*global_data.comm_ids, true)));

        global_data.default_coll_attr.reset(new iccl_coll_attr_t{});
        global_data.default_coll_attr->to_cache = 1;

        if (env_data.out_of_order_support)
        {
            global_data.ooo_manager =
                std::unique_ptr<out_of_order::ooo_match>(
                    new out_of_order::ooo_match(*global_data.executor, *global_data.comm_ids));
        }

        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

iccl_status_t iccl_finalize()
{
    try
    {
        global_data.sched_cache.reset();
        global_data.ooo_manager.reset();
        global_data.executor.reset();
        global_data.fusion_manager.reset();
        global_data.comm.reset();
        global_data.comm_ids.reset();
        global_data.atl_tag.reset();
        global_data.parallelizer.reset();
        global_data.default_coll_attr.reset();

        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

iccl_status_t ICCL_API iccl_wait(iccl_request_t req)
{
    try
    {
        if (!req)
        {
            LOG_ERROR("empty request");
            return iccl_status_success;
        }

        auto request = static_cast<iccl_request*>(req);

        iccl_wait_impl(global_data.executor.get(), request);

        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

iccl_status_t ICCL_API iccl_test(iccl_request_t req,
                                 int* is_completed)
{
    try
    {
        if (!req)
        {
            LOG_ERROR("empty request");
            if (is_completed)
            {
                *is_completed = 1;
            }
            return iccl_status_success;
        }

        auto request = static_cast<iccl_request*>(req);

        auto completed = iccl_test_impl(global_data.executor.get(), request);

        *is_completed = static_cast<int>(completed);

        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

iccl_status_t iccl_comm_create(iccl_comm_t* comm_t,
                               iccl_comm_attr_t* comm_attr)
{
    ICCL_ASSERT(comm_t);
    try
    {
        iccl_comm* comm = nullptr;
        if (!comm_attr)
        {
            LOG_DEBUG("Duplicating global comm");
            comm = new iccl_comm(global_data.comm->rank(), global_data.comm->size(),
                                 std::unique_ptr<comm_id>(new comm_id(*global_data.comm_ids)));
        }
        else
        {

            comm = iccl_comm::create_with_color(comm_attr->color, global_data.comm_ids.get(), global_data.comm.get());
        }

        *comm_t = static_cast<void*>(comm);
        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

iccl_status_t iccl_comm_free(iccl_comm_t comm_t)
{
    ICCL_ASSERT(comm_t);
    LOG_DEBUG("Free comm ", comm_t);
    try
    {
        auto comm = static_cast<iccl_comm*>(comm_t);
        delete comm;
        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

iccl_status_t ICCL_API iccl_get_comm_rank(iccl_comm_t comm_t,
                                          size_t* out_rank)
{
    try
    {
        auto comm = static_cast<iccl_comm*>(comm_t);
        auto rank = comm ? comm->rank() : global_data.comm->rank();
        if (out_rank)
        {
            *out_rank = rank;
        }
        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

iccl_status_t ICCL_API iccl_get_comm_size(iccl_comm_t comm_t,
                                          size_t* out_size)
{
    try
    {
        auto comm = static_cast<iccl_comm*>(comm_t);
        auto size = comm ? comm->size() : global_data.comm->size();
        if (out_size)
        {
            *out_size = size;
        }
        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

iccl_status_t ICCL_API iccl_bcast(
    void* buf,
    size_t count,
    iccl_datatype_t dtype,
    size_t root,
    const iccl_coll_attr_t* attributes,
    iccl_comm_t communicator,
    iccl_request_t* req)
{
    try
    {
        if (!req)
        {
            return iccl_status_invalid_arguments;
        }

        auto comm = static_cast<iccl_comm*>(communicator);
        iccl_comm* real_comm = comm ?: global_data.comm.get();

        auto request = iccl_bcast_impl(buf, count, dtype, root, attributes, real_comm);
        *req = static_cast<iccl_request_t>(request);

        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

iccl_status_t ICCL_API iccl_reduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    iccl_datatype_t dtype,
    iccl_reduction_t reduction,
    size_t root,
    const iccl_coll_attr_t* attributes,
    iccl_comm_t communicator,
    iccl_request_t* req)
{
    try
    {
        if (!req)
        {
            return iccl_status_invalid_arguments;
        }

        auto comm = static_cast<iccl_comm*>(communicator);
        iccl_comm* real_comm = comm ?: global_data.comm.get();

        auto request = iccl_reduce_impl(send_buf, recv_buf, count, dtype, reduction, root, attributes, real_comm);
        *req = static_cast<iccl_request_t>(request);

        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

iccl_status_t ICCL_API iccl_allreduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    iccl_datatype_t dtype,
    iccl_reduction_t reduction,
    const iccl_coll_attr_t* attributes,
    iccl_comm_t communicator,
    iccl_request_t* req)
{
    try
    {
        if (!req)
        {
            return iccl_status_invalid_arguments;
        }

        auto comm = static_cast<iccl_comm*>(communicator);
        iccl_comm* real_comm = comm ?: global_data.comm.get();

        auto request = iccl_allreduce_impl(send_buf, recv_buf, count, dtype, reduction, attributes, real_comm);
        *req = static_cast<iccl_request_t>(request);

        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

iccl_status_t ICCL_API iccl_allgatherv(
    const void* send_buf,
    size_t send_count,
    void* recv_buf,
    size_t* recv_counts,
    iccl_datatype_t dtype,
    const iccl_coll_attr_t* attributes,
    iccl_comm_t communicator,
    iccl_request_t* req)
{
    try
    {
        if (!req)
        {
            return iccl_status_invalid_arguments;
        }

        auto comm = static_cast<iccl_comm*>(communicator);
        iccl_comm* real_comm = comm ?: global_data.comm.get();

        auto request = iccl_allgatherv_impl(send_buf, send_count, recv_buf, recv_counts, dtype, attributes, real_comm);
        *req = static_cast<iccl_request_t>(request);

        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

iccl_status_t ICCL_API iccl_sparse_allreduce(const void* send_ind_buf,
                                             size_t send_ind_count,
                                             const void* send_val_buf,
                                             size_t send_val_count,
                                             void** recv_ind_buf,
                                             size_t* recv_ind_count,
                                             void** recv_val_buf,
                                             size_t* recv_val_count,
                                             iccl_datatype_t index_dtype,
                                             iccl_datatype_t dtype,
                                             iccl_reduction_t reduction,
                                             const iccl_coll_attr_t* attributes,
                                             iccl_comm_t communicator,
                                             iccl_request_t* req)
{
    try
    {
        if (!req)
        {
            return iccl_status_invalid_arguments;
        }

        auto comm = static_cast<iccl_comm*>(communicator);
        iccl_comm* real_comm = comm ?: global_data.comm.get();

        auto request = iccl_sparse_allreduce_impl(send_ind_buf, send_ind_count, send_val_buf, send_val_count,
                                                  recv_ind_buf, recv_ind_count, recv_val_buf,
                                                  recv_val_count, index_dtype, dtype, reduction, attributes, real_comm);
        *req = static_cast<iccl_request_t>(request);

        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

iccl_status_t ICCL_API iccl_barrier(iccl_comm_t communicator)
{
    try
    {
        auto comm = static_cast<iccl_comm*>(communicator);
        iccl_comm* real_comm = comm ?: global_data.comm.get();

        iccl_barrier_impl(real_comm);

        return iccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}
