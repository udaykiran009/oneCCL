#include "common/request/request.hpp"
#include "common/request/host_request.hpp"
#include "exec/exec.hpp"
namespace ccl
{
host_request_impl::host_request_impl(ccl_request* r) : req(r)
{
    if (!req)
    {
        // If the user calls collective with coll_attr->synchronous=1 then it will be progressed
        // in place and API will return null request. In this case mark cpp wrapper as completed,
        // all calls to wait() or test() will do nothing
        completed = true;
    }
}

host_request_impl::~host_request_impl()
{
    if (!completed)
    {
        LOG_ERROR("not completed request is destroyed");
    }
}

void host_request_impl::wait()
{
    if (!completed)
    {
        ccl_wait_impl(ccl::global_data::get().executor.get(), req);
        completed = true;
    }
}

bool host_request_impl::test()
{
    if (!completed)
    {
        completed = ccl_test_impl(ccl::global_data::get().executor.get(), req);
    }
    return completed;
}

bool host_request_impl::cancel()
{
    throw ccl_error(std::string(__FUNCTION__) + " - is not implemented");
}

event& host_request_impl::get_event()
{
    throw ccl_error(std::string(__FUNCTION__) + " - is not implemented");
}
}
