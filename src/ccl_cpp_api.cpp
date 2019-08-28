#include "ccl.hpp"
#include "common/global/global.hpp"
#include "exec/exec.hpp"

#define CCL_CHECK_AND_THROW(result, diagnostic)   \
  do {                                            \
      if (result != ccl_status_success)           \
      {                                           \
          throw ccl_error(diagnostic);            \
      }                                           \
  } while (0);

namespace ccl
{
class request_impl final : public request
{
public:
    explicit request_impl(ccl_request* r) : req(r)
    {
        if (!req)
        {
            // If the user calls collective with coll_attr->synchronous=1 then it will be progressed
            // in place and API will return null request. In this case mark cpp wrapper as completed,
            // all calls to wait() or test() will do nothing
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

bool CCL_API ccl::environment::is_initialized = false;

CCL_API ccl::environment::environment()
{
    if (!is_initialized)
    {
        ccl_status_t result = ccl_init();
        if (result == ccl_status_success)
        {
            is_initialized = true;
        }
        CCL_CHECK_AND_THROW(result, "failed to initialize ccl");
    }
    else
    {
        throw ccl::ccl_error("already initialized");
    }
}

CCL_API void ccl::environment::set_resize_fn(ccl_resize_fn_t callback)
{
    if (is_initialized)
    {
        ccl_status_t result = ccl_set_resize_fn(callback);
        CCL_CHECK_AND_THROW(result, "failed to set callback");
    }
    else
    {
        throw ccl::ccl_error("not initialized");
    }
}

CCL_API ccl::environment::~environment()
{
    if (is_initialized)
    {
        auto result = ccl_finalize();
        if (result != ccl_status_success)
        {
            abort();
        }
        is_initialized = false;
    }
}

CCL_API ccl::stream::stream()
{
    this->stream_impl = std::make_shared<ccl_stream>(static_cast<ccl_stream_type_t>(ccl::stream_type::cpu),
                                                     nullptr /* native_stream */);
}

CCL_API ccl::stream::stream(ccl::stream_type type, void* native_stream)
{
    this->stream_impl = std::make_shared<ccl_stream>(static_cast<ccl_stream_type_t>(type),
                                                     native_stream);
}

CCL_API ccl::communicator::communicator()
{
    comm_impl = global_data.comm;
}

CCL_API ccl::communicator::communicator(const ccl::comm_attr* attr)
{
    if (!attr)
    {
        comm_impl = global_data.comm;
    }
    else
    {
        comm_impl = std::shared_ptr<ccl_comm>(
            ccl_comm::create_with_color(attr->color,
                                        global_data.comm_ids.get(),
                                        global_data.comm.get()));
    }
}

CCL_API size_t ccl::communicator::rank()
{
    return comm_impl->rank();
}

CCL_API size_t ccl::communicator::size()
{
    return comm_impl->size();
}

std::shared_ptr<ccl::request> CCL_API
ccl::communicator::allgatherv(const void* send_buf,
                              size_t send_count,
                              void* recv_buf,
                              const size_t* recv_counts,
                              ccl::data_type dtype,
                              const ccl::coll_attr* attr,
                              const ccl::stream* stream)
{
    ccl_request* req = ccl_allgatherv_impl(send_buf, send_count, recv_buf, recv_counts,
                                           static_cast<ccl_datatype_t>(dtype),
                                           attr, comm_impl.get(),
                                           (stream) ? stream->stream_impl.get() : nullptr);
    return std::make_shared<ccl::request_impl>(req);
}

std::shared_ptr<ccl::request> CCL_API
ccl::communicator::allreduce(const void* send_buf,
                             void* recv_buf,
                             size_t count,
                             ccl::data_type dtype,
                             ccl::reduction reduction,
                             const ccl::coll_attr* attr,
                             const ccl::stream* stream)
{
    ccl_request* req = ccl_allreduce_impl(send_buf, recv_buf, count,
                                          static_cast<ccl_datatype_t>(dtype),
                                          static_cast<ccl_reduction_t>(reduction),
                                          attr, comm_impl.get(),
                                          (stream) ? stream->stream_impl.get() : nullptr);
    return std::make_shared<ccl::request_impl>(req);
}

void CCL_API ccl::communicator::barrier(const ccl::stream* stream)
{
    ccl_barrier_impl(comm_impl.get(),
                     (stream) ? stream->stream_impl.get() : nullptr);
}

std::shared_ptr<ccl::request> CCL_API
ccl::communicator::bcast(void* buf,
                         size_t count,
                         ccl::data_type dtype,
                         size_t root,
                         const ccl::coll_attr* attr,
                         const ccl::stream* stream)
{
    ccl_request* req = ccl_bcast_impl(buf, count,
                                      static_cast<ccl_datatype_t>(dtype),
                                      root, attr, comm_impl.get(),
                                      (stream) ? stream->stream_impl.get() : nullptr);
    return std::make_shared<ccl::request_impl>(req);
}

std::shared_ptr<ccl::request> CCL_API
ccl::communicator::reduce(const void* send_buf,
                          void* recv_buf,
                          size_t count,
                          ccl::data_type dtype,
                          ccl::reduction reduction,
                          size_t root,
                          const ccl::coll_attr* attr,
                          const ccl::stream* stream)
{
    ccl_request* req = ccl_reduce_impl(send_buf, recv_buf, count,
                                       static_cast<ccl_datatype_t>(dtype),
                                       static_cast<ccl_reduction_t>(reduction),
                                       root, attr, comm_impl.get(),
                                       (stream) ? stream->stream_impl.get() : nullptr);
    return std::make_shared<ccl::request_impl>(req);
}



std::shared_ptr<ccl::request> CCL_API
ccl::communicator::sparse_allreduce(const void* send_ind_buf, size_t send_ind_count,
                                    const void* send_val_buf, size_t send_val_count,
                                    void** recv_ind_buf, size_t* recv_ind_count,
                                    void** recv_val_buf, size_t* recv_val_count,
                                    ccl::data_type index_dtype,
                                    ccl::data_type value_dtype,
                                    ccl::reduction reduction,
                                    const ccl::coll_attr* attr,
                                    const ccl::stream* stream)
{
    ccl_request* req = ccl_sparse_allreduce_impl(send_ind_buf, send_ind_count,
                                                 send_val_buf, send_val_count,
                                                 recv_ind_buf, recv_ind_count,
                                                 recv_val_buf, recv_val_count,
                                                 static_cast<ccl_datatype_t>(index_dtype),
                                                 static_cast<ccl_datatype_t>(value_dtype),
                                                 static_cast<ccl_reduction_t>(reduction),
                                                 attr, comm_impl.get(),
                                                 (stream) ? stream->stream_impl.get() : nullptr);
    return std::make_shared<ccl::request_impl>(req);
}
