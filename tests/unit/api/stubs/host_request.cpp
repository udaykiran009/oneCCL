#include "common/request/request.hpp"
#include "common/request/host_request.hpp"

namespace ccl
{
host_request_impl::host_request_impl(ccl_request* r) : req(r)
{
    (void)req;
}

host_request_impl::~host_request_impl()
{
}

void host_request_impl::wait()
{
}

bool host_request_impl::test()
{
    (void)completed;
    return true;
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
