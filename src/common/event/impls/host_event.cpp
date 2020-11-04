#include "common/request/request.hpp"
#include "common/event/impls/host_event.hpp"
#include "exec/exec.hpp"

namespace ccl {

host_event_impl::host_event_impl(ccl_request* r) : req(r) {
    if (!req) {
        // If the user calls collective with coll_attr->synchronous=1 then it will be progressed
        // in place and API will return null event. In this case mark cpp wrapper as completed,
        // all calls to wait() or test() will do nothing
        completed = true;
    }
}

host_event_impl::~host_event_impl() {
    if (!completed) {
        LOG_ERROR("not completed event is destroyed");
    }
}

void host_event_impl::wait() {
    if (!completed) {
        ccl_wait_impl(ccl::global_data::get().executor.get(), req);
        completed = true;
    }
}

bool host_event_impl::test() {
    if (!completed) {
        completed = ccl_test_impl(ccl::global_data::get().executor.get(), req);
    }
    return completed;
}

bool host_event_impl::cancel() {
    throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
}

event::native_t& host_event_impl::get_native() {
    throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
}

} // namespace ccl
