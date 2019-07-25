#include "ccl.hpp"
#include "common/global/global.hpp"
#include "common/request/request.hpp"
#include "exec/exec.hpp"

#define CHECK_AND_THROW(result, diagnostic)     \
do {                                            \
    if(result != ccl_status_success)            \
    {                                           \
        throw ccl_error(diagnostic);            \
    }                                           \
} while (0);

namespace ccl
{
class request_impl final : public request
{
public:
    explicit request_impl(ccl_request* ccl_req) : req(ccl_req)
    {
        if (!req)
        {
            //if the user calls synchronous collective op via ccl_coll_attr_t->synchronous=1 then it will be progressed
            //in place and API will return null ccl_request_t. In this case we mark cpp wrapper as completed,
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
            ccl_wait_impl(global_data.executor.get(), req);
            completed = true;
        }
    }

    bool test() final
    {
        if (!completed)
        {
            completed = ccl_test_impl(global_data.executor.get(), req);
        }
        return completed;
    }

private:
    ccl_request* req = nullptr;
    bool completed = false;
};
}

using namespace ccl;

bool CCL_API environment::is_initialized = false;

CCL_API environment::environment()
{
    if (!is_initialized)
    {
        //todo avoid try/carch block in ccl_init
        ccl_status_t result = ccl_init();
        if (result == ccl_status_success)
        {
            is_initialized = true;
        }
        CHECK_AND_THROW(result, "failed to initialize ccl");
    }
    else
    {
        throw ccl::ccl_error("already initialized");
    }
}

CCL_API environment::~environment()
{
    if (is_initialized)
    {
        auto result = ccl_finalize();
        if(result != ccl_status_success)
        {
            abort();
        }
        is_initialized = false;
    }
}

CCL_API stream::stream()
{
    this->stream_impl = std::make_shared<ccl_stream>(ccl_stream_cpu, nullptr);
}

CCL_API stream::stream(ccl_stream_type_t stream_type, void* native_stream)
{
    this->stream_impl = std::make_shared<ccl_stream>(stream_type, native_stream);
}

CCL_API communicator::communicator()
{
    //default constructor uses global communicator
    //todo: possibly we should introduce some proxy/getter for global data class to make unit testing easier
    comm_impl = global_data.comm;
}

CCL_API communicator::communicator(ccl_comm_attr_t* comm_attr)
{
    if (!comm_attr)
    {
        comm_impl = global_data.comm;
    }
    else
    {
        comm_impl = std::shared_ptr<ccl_comm>(
            ccl_comm::create_with_color(comm_attr->color, global_data.comm_ids.get(), global_data.comm.get()));
    }
}

CCL_API size_t communicator::rank()
{
    return comm_impl->rank();
}

CCL_API size_t communicator::size()
{
    return comm_impl->size();
}

std::shared_ptr<ccl::request> CCL_API communicator::bcast(void* buf,
                                                          size_t count,
                                                          ccl::data_type dtype,
                                                          size_t root,
                                                          const ccl_coll_attr_t* attributes,
                                                          const ccl::stream* stream)
{

    auto ccl_data_type = static_cast<ccl_datatype_t>(dtype);

    ccl_request* ccl_req = ccl_bcast_impl(buf, count, ccl_data_type, root, attributes, comm_impl.get(), stream->get_stream());

    return std::make_shared<ccl::request_impl>(ccl_req);
}

std::shared_ptr<ccl::request> CCL_API communicator::reduce(const void* send_buf,
                                                           void* recv_buf,
                                                           size_t count,
                                                           ccl::data_type dtype,
                                                           ccl::reduction reduction,
                                                           size_t root,
                                                           const ccl_coll_attr_t* attributes,
                                                           const ccl::stream* stream)
{

    auto ccl_data_type = static_cast<ccl_datatype_t>(dtype);
    auto ccl_reduction_type = static_cast<ccl_reduction_t>(reduction);

    ccl_request* ccl_req = ccl_reduce_impl(send_buf, recv_buf, count, ccl_data_type, ccl_reduction_type,
                                           root, attributes, comm_impl.get(), stream->get_stream());

    return std::make_shared<ccl::request_impl>(ccl_req);
}

std::shared_ptr<ccl::request> CCL_API communicator::allreduce(const void* send_buf,
                                                              void* recv_buf,
                                                              size_t count,
                                                              ccl::data_type dtype,
                                                              ccl::reduction reduction,
                                                              const ccl_coll_attr_t* attributes,
                                                              const ccl::stream* stream)
{
    auto ccl_data_type = static_cast<ccl_datatype_t>(dtype);
    auto ccl_reduction_type = static_cast<ccl_reduction_t>(reduction);

    ccl_request* ccl_req = ccl_allreduce_impl(send_buf, recv_buf, count, ccl_data_type, ccl_reduction_type,
                                              attributes, comm_impl.get(), stream->get_stream());

    return std::make_shared<ccl::request_impl>(ccl_req);
}

std::shared_ptr<ccl::request> CCL_API communicator::allgatherv(const void* send_buf,
                                                               size_t send_count,
                                                               void* recv_buf,
                                                               size_t* recv_counts,
                                                               ccl::data_type dtype,
                                                               const ccl_coll_attr_t* attributes,
                                                               const ccl::stream* stream)
{
    auto ccl_data_type = static_cast<ccl_datatype_t>(dtype);

    ccl_request* ccl_req = ccl_allgatherv_impl(send_buf, send_count, recv_buf, recv_counts, ccl_data_type,
                                               attributes, comm_impl.get(), stream->get_stream());

    return std::make_shared<ccl::request_impl>(ccl_req);
}

std::shared_ptr<ccl::request> CCL_API communicator::sparse_allreduce(const void* send_ind_buf,
                                                                     size_t send_ind_count,
                                                                     const void* send_val_buf,
                                                                     size_t send_val_count,
                                                                     void** recv_ind_buf,
                                                                     size_t* recv_ind_count,
                                                                     void** recv_val_buf,
                                                                     size_t* recv_val_count,
                                                                     ccl::data_type index_dtype,
                                                                     ccl::data_type value_dtype,
                                                                     ccl::reduction reduction,
                                                                     const ccl_coll_attr_t* attributes,
                                                                     const ccl::stream* stream)
{
    auto ccl_index_type = static_cast<ccl_datatype_t>(index_dtype);
    auto ccl_data_type = static_cast<ccl_datatype_t>(value_dtype);
    auto ccl_reduction_type = static_cast<ccl_reduction_t>(reduction);

    ccl_request* ccl_req = ccl_sparse_allreduce_impl(send_ind_buf, send_ind_count, send_val_buf, send_val_count,
                                                     recv_ind_buf, recv_ind_count, recv_val_buf, recv_val_count,
                                                     ccl_index_type, ccl_data_type,
                                                     ccl_reduction_type, attributes, comm_impl.get(), stream->get_stream());

    return std::make_shared<ccl::request_impl>(ccl_req);
}

void CCL_API communicator::barrier(const ccl::stream* stream)
{
    ccl_barrier_impl(comm_impl.get(), stream->get_stream());
}
