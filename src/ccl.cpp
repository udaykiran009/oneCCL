#include "exec/exec.hpp"
#include "out_of_order/ooo_match.hpp"
#include "fusion/fusion.hpp"
#include "parallelizer/parallelizer.hpp"
#include "sched/sched_cache.hpp"
#include "common/utils/tree.hpp"
#include "common/comm/atl_tag.hpp"
#include "common/stream/stream.hpp"

ccl_status_t ccl_init()
{
    try
    {
        ccl_env_parse();
        ccl_datatype_init();

        global_data.sched_cache = std::unique_ptr<ccl_sched_cache>(new ccl_sched_cache());
        global_data.parallelizer = std::unique_ptr<ccl_parallelizer>(new ccl_parallelizer(env_data.worker_count));

        if (env_data.enable_fusion)
        {
            global_data.fusion_manager =
                std::unique_ptr<ccl_fusion_manager>(new ccl_fusion_manager());
        }

        global_data.executor = std::unique_ptr<ccl_executor>(new ccl_executor(env_data, global_data));

        if (global_data.executor->proc_idx == 0)
        {
            ccl_env_print();
        }

        global_data.atl_tag = std::unique_ptr<ccl_atl_tag>(new ccl_atl_tag(global_data.executor->tag_bits,
                                                                             global_data.executor->max_tag));

        global_data.comm_ids = std::unique_ptr<ccl_comm_id_storage>(new ccl_comm_id_storage(ccl_comm::max_comm_count));

        global_data.comm = std::make_shared<ccl_comm>(global_data.executor->proc_idx,
                                                      global_data.executor->proc_count,
                                                      std::unique_ptr<comm_id>(
                                                           new comm_id(*global_data.comm_ids, true)));

        global_data.default_coll_attr.reset(new ccl_coll_attr_t{});
        global_data.default_coll_attr->to_cache = 1;

        if (env_data.out_of_order_support)
        {
            global_data.ooo_manager =
                std::unique_ptr<out_of_order::ooo_match>(
                    new out_of_order::ooo_match(*global_data.executor, *global_data.comm_ids));
        }

        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t ccl_finalize()
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

        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t CCL_API ccl_wait(ccl_request_t req)
{
    try
    {
        if (!req)
        {
            LOG_ERROR("empty request");
            return ccl_status_success;
        }

        auto request = static_cast<ccl_request*>(req);

        ccl_wait_impl(global_data.executor.get(), request);

        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t CCL_API ccl_test(ccl_request_t req,
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
            return ccl_status_success;
        }

        auto request = static_cast<ccl_request*>(req);

        auto completed = ccl_test_impl(global_data.executor.get(), request);

        *is_completed = static_cast<int>(completed);

        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t ccl_comm_create(ccl_comm_t* comm_t,
                               ccl_comm_attr_t* comm_attr)
{
    CCL_ASSERT(comm_t);
    try
    {
        ccl_comm* comm = nullptr;
        if (!comm_attr)
        {
            LOG_DEBUG("Duplicating global comm");
            comm = new ccl_comm(global_data.comm->rank(), global_data.comm->size(),
                                std::unique_ptr<comm_id>(new comm_id(*global_data.comm_ids)));
        }
        else
        {

            comm = ccl_comm::create_with_color(comm_attr->color, global_data.comm_ids.get(), global_data.comm.get());
        }

        *comm_t = static_cast<void*>(comm);
        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t ccl_comm_free(ccl_comm_t comm_t)
{
    CCL_ASSERT(comm_t);
    LOG_DEBUG("Free comm ", comm_t);
    try
    {
        auto comm = static_cast<ccl_comm*>(comm_t);
        delete comm;
        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t CCL_API ccl_get_comm_rank(ccl_comm_t comm_t,
                                          size_t* out_rank)
{
    try
    {
        auto comm = static_cast<ccl_comm*>(comm_t);
        auto rank = comm ? comm->rank() : global_data.comm->rank();
        if (out_rank)
        {
            *out_rank = rank;
        }
        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t CCL_API ccl_get_comm_size(ccl_comm_t comm_t,
                                          size_t* out_size)
{
    try
    {
        auto comm = static_cast<ccl_comm*>(comm_t);
        auto size = comm ? comm->size() : global_data.comm->size();
        if (out_size)
        {
            *out_size = size;
        }
        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t ccl_stream_create(ccl_stream_type_t stream_type,
                               void* native_stream,
                               ccl_stream_t* ccl_stream_out)
{
    CCL_ASSERT(ccl_stream_out);
    try
    {
        LOG_DEBUG("Create stream");
        *ccl_stream_out = static_cast<void*>(new ccl_stream(stream_type, native_stream));
        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t ccl_stream_free(ccl_stream_t ccl_stream_in)
{
    CCL_ASSERT(ccl_stream_in);
    LOG_DEBUG("Free stream ", ccl_stream_in);
    try
    {
        delete static_cast<const ccl_stream*>(ccl_stream_in);

        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t CCL_API ccl_bcast(
    void* buf,
    size_t count,
    ccl_datatype_t dtype,
    size_t root,
    const ccl_coll_attr_t* attributes,
    ccl_comm_t communicator,
    ccl_stream_t stream,
    ccl_request_t* req)
{
    try
    {
        if (!req)
        {
            return ccl_status_invalid_arguments;
        }

        auto comm = static_cast<ccl_comm*>(communicator);
        ccl_comm* real_comm = comm ?: global_data.comm.get();

        auto real_stream = static_cast<const ccl_stream*>(stream);

        auto request = ccl_bcast_impl(buf, count, dtype, root, attributes, real_comm, real_stream);
        *req = static_cast<ccl_request_t>(request);

        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t CCL_API ccl_reduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    ccl_datatype_t dtype,
    ccl_reduction_t reduction,
    size_t root,
    const ccl_coll_attr_t* attributes,
    ccl_comm_t communicator,
    ccl_stream_t stream,
    ccl_request_t* req)
{
    try
    {
        if (!req)
        {
            return ccl_status_invalid_arguments;
        }

        auto comm = static_cast<ccl_comm*>(communicator);
        ccl_comm* real_comm = comm ?: global_data.comm.get();

        auto real_stream = static_cast<const ccl_stream*>(stream);

        auto request = ccl_reduce_impl(send_buf, recv_buf, count, dtype, reduction, root, attributes, real_comm, real_stream);
        *req = static_cast<ccl_request_t>(request);

        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t CCL_API ccl_allreduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    ccl_datatype_t dtype,
    ccl_reduction_t reduction,
    const ccl_coll_attr_t* attributes,
    ccl_comm_t communicator,
    ccl_stream_t stream,
    ccl_request_t* req)
{
    try
    {
        if (!req)
        {
            return ccl_status_invalid_arguments;
        }

        auto comm = static_cast<ccl_comm*>(communicator);
        ccl_comm* real_comm = comm ?: global_data.comm.get();

        auto real_stream = static_cast<const ccl_stream*>(stream);

        auto request = ccl_allreduce_impl(send_buf, recv_buf, count, dtype, reduction, attributes, real_comm, real_stream);
        *req = static_cast<ccl_request_t>(request);

        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t CCL_API ccl_allgatherv(
    const void* send_buf,
    size_t send_count,
    void* recv_buf,
    size_t* recv_counts,
    ccl_datatype_t dtype,
    const ccl_coll_attr_t* attributes,
    ccl_comm_t communicator,
    ccl_stream_t stream,
    ccl_request_t* req)
{
    try
    {
        if (!req)
        {
            return ccl_status_invalid_arguments;
        }

        auto comm = static_cast<ccl_comm*>(communicator);
        ccl_comm* real_comm = comm ?: global_data.comm.get();

        auto real_stream = static_cast<const ccl_stream*>(stream);

        auto request = ccl_allgatherv_impl(send_buf, send_count, recv_buf, recv_counts, dtype, attributes, real_comm, real_stream);
        *req = static_cast<ccl_request_t>(request);

        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t CCL_API ccl_sparse_allreduce(const void* send_ind_buf,
                                          size_t send_ind_count,
                                          const void* send_val_buf,
                                          size_t send_val_count,
                                          void** recv_ind_buf,
                                          size_t* recv_ind_count,
                                          void** recv_val_buf,
                                          size_t* recv_val_count,
                                          ccl_datatype_t index_dtype,
                                          ccl_datatype_t dtype,
                                          ccl_reduction_t reduction,
                                          const ccl_coll_attr_t* attributes,
                                          ccl_comm_t communicator,
                                          ccl_stream_t stream,
                                          ccl_request_t* req)
{
    try
    {
        if (!req)
        {
            return ccl_status_invalid_arguments;
        }

        auto comm = static_cast<ccl_comm*>(communicator);
        ccl_comm* real_comm = comm ?: global_data.comm.get();

        auto real_stream = static_cast<const ccl_stream*>(stream);

        auto request = ccl_sparse_allreduce_impl(send_ind_buf, send_ind_count, send_val_buf, send_val_count,
                                                 recv_ind_buf, recv_ind_count, recv_val_buf,
                                                 recv_val_count, index_dtype, dtype, reduction, attributes, real_comm, real_stream);
        *req = static_cast<ccl_request_t>(request);

        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}

ccl_status_t CCL_API ccl_barrier(ccl_comm_t communicator,
                                 ccl_stream_t stream)
{
    try
    {
        auto comm = static_cast<ccl_comm*>(communicator);
        ccl_comm* real_comm = comm ?: global_data.comm.get();

        auto real_stream = static_cast<const ccl_stream*>(stream);

        ccl_barrier_impl(real_comm, real_stream);

        return ccl_status_success;
    }
    COMMON_CATCH_BLOCK();
}
