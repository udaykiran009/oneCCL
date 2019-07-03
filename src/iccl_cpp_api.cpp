#include "iccl.hpp"
#include "common/global/global.hpp"
#include "common/request/request.hpp"
#include "exec/exec.hpp"

#define CHECK_AND_THROW(result, diagnostic)     \
do {                                            \
    if(result != iccl_status_success)           \
    {                                           \
        throw iccl_error(diagnostic);           \
    }                                           \
} while (0);

namespace iccl
{
class request_impl final : public request
{
public:
    explicit request_impl(iccl_request* iccl_req) : req(iccl_req)
    {
        if (!req)
        {
            //if the user calls synchronous collective op via iccl_coll_attr_t->synchronous=1 then it will be progressed
            //in place and API will return null iccl_request_t. In this case we mark cpp wrapper as completed,
            //all calls to wait() or test() will do nothing
            completed = true;
        }
    }

    ~request_impl()
    {
        if (!completed)
        {
            LOG_ERROR("not completed request is destroyed");
        }
    }

    void wait() final
    {
        if (!completed)
        {
            iccl_wait_impl(global_data.executor.get(), req);
            completed = true;
        }
    }

    bool test() final
    {
        if (!completed)
        {
            completed = iccl_test_impl(global_data.executor.get(), req);
        }
        return completed;
    }

private:
    iccl_request* req = nullptr;
    bool completed = false;
};
}

using namespace iccl;

bool ICCL_API environment::is_initialized = false;

ICCL_API environment::environment()
{
    if (!is_initialized)
    {
        //todo avoid try/carch block in iccl_init
        iccl_status_t result = iccl_init();
        if (result == iccl_status_success)
        {
            is_initialized = true;
        }
        CHECK_AND_THROW(result, "failed to initialize iccl");
    }
    else
    {
        throw iccl::iccl_error("already initialized");
    }
}

ICCL_API environment::~environment()
{
    if (is_initialized)
    {
        auto result = iccl_finalize();
        if(result != iccl_status_success)
        {
            abort();
        }
        is_initialized = false;
    }
}

ICCL_API communicator::communicator()
{
    //default constructor uses global communicator
    //todo: possibly we should introduce some proxy/getter for global data class to make unit testing easier
    comm_impl = global_data.comm;
}

ICCL_API communicator::communicator(iccl_comm_attr_t* comm_attr)
{
    if (!comm_attr)
    {
        comm_impl = global_data.comm;
    }
    else
    {
        comm_impl = std::shared_ptr<iccl_comm>(
            iccl_comm::create_with_color(comm_attr->color, global_data.comm_ids.get(), global_data.comm.get()));
    }
}

ICCL_API size_t communicator::rank()
{
    return comm_impl->rank();
}

ICCL_API size_t communicator::size()
{
    return comm_impl->size();
}

std::shared_ptr<iccl::request> ICCL_API communicator::bcast(void* buf,
                                                            size_t count,
                                                            iccl::data_type dtype,
                                                            size_t root,
                                                            const iccl_coll_attr_t* attributes)
{

    auto iccl_data_type = static_cast<iccl_datatype_t>(dtype);

    iccl_request* iccl_req = iccl_bcast_impl(buf, count, iccl_data_type, root, attributes, comm_impl.get());

    return std::make_shared<iccl::request_impl>(iccl_req);
}

std::shared_ptr<iccl::request> ICCL_API communicator::reduce(const void* send_buf,
                                                             void* recv_buf,
                                                             size_t count,
                                                             iccl::data_type dtype,
                                                             iccl::reduction reduction,
                                                             size_t root,
                                                             const iccl_coll_attr_t* attributes)
{

    auto iccl_data_type = static_cast<iccl_datatype_t>(dtype);
    auto iccl_reduction_type = static_cast<iccl_reduction_t>(reduction);

    iccl_request* iccl_req = iccl_reduce_impl(send_buf, recv_buf, count, iccl_data_type, iccl_reduction_type,
                                              root, attributes, comm_impl.get());

    return std::make_shared<iccl::request_impl>(iccl_req);
}

std::shared_ptr<iccl::request> ICCL_API communicator::allreduce(const void* send_buf,
                                                                void* recv_buf,
                                                                size_t count,
                                                                iccl::data_type dtype,
                                                                iccl::reduction reduction,
                                                                const iccl_coll_attr_t* attributes)
{
    auto iccl_data_type = static_cast<iccl_datatype_t>(dtype);
    auto iccl_reduction_type = static_cast<iccl_reduction_t>(reduction);

    iccl_request* iccl_req = iccl_allreduce_impl(send_buf, recv_buf, count, iccl_data_type, iccl_reduction_type,
                                                 attributes, comm_impl.get());

    return std::make_shared<iccl::request_impl>(iccl_req);
}

std::shared_ptr<iccl::request> ICCL_API communicator::allgatherv(const void* send_buf,
                                                                 size_t send_count,
                                                                 void* recv_buf,
                                                                 size_t* recv_counts,
                                                                 iccl::data_type dtype,
                                                                 const iccl_coll_attr_t* attributes)
{
    auto iccl_data_type = static_cast<iccl_datatype_t>(dtype);

    iccl_request* iccl_req = iccl_allgatherv_impl(send_buf, send_count, recv_buf, recv_counts, iccl_data_type,
                                                  attributes, comm_impl.get());

    return std::make_shared<iccl::request_impl>(iccl_req);
}

std::shared_ptr<iccl::request> ICCL_API communicator::sparse_allreduce(const void* send_ind_buf,
                                                                       size_t send_ind_count,
                                                                       const void* send_val_buf,
                                                                       size_t send_val_count,
                                                                       void** recv_ind_buf,
                                                                       size_t* recv_ind_count,
                                                                       void** recv_val_buf,
                                                                       size_t* recv_val_count,
                                                                       iccl::data_type index_dtype,
                                                                       iccl::data_type value_dtype,
                                                                       iccl::reduction reduction,
                                                                       const iccl_coll_attr_t* attributes)
{
    auto iccl_index_type = static_cast<iccl_datatype_t>(index_dtype);
    auto iccl_data_type = static_cast<iccl_datatype_t>(value_dtype);
    auto iccl_reduction_type = static_cast<iccl_reduction_t>(reduction);

    iccl_request* iccl_req = iccl_sparse_allreduce_impl(send_ind_buf, send_ind_count, send_val_buf, send_val_count,
                                                        recv_ind_buf, recv_ind_count, recv_val_buf, recv_val_count,
                                                        iccl_index_type, iccl_data_type,
                                                        iccl_reduction_type, attributes, comm_impl.get());

    return std::make_shared<iccl::request_impl>(iccl_req);
}

void ICCL_API communicator::barrier()
{
    iccl_barrier_impl(comm_impl.get());
}
