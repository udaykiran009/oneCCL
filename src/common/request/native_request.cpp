#include "common/request/native_request.hpp"
#include "common/log/log.hpp"

namespace ccl {

native_request_impl::native_request_impl(event::native_t& native_event, ccl::library_version version)
    : ev(new ccl_event(native_event, version)) {
}

native_request_impl::~native_request_impl() {
    if (!completed) {
        LOG_ERROR("not completed event is destroyed");
    }
}

void native_request_impl::wait() {
    if (!completed) {
        #ifdef CCL_ENABLE_SYCL
            auto native_event = ev->get_attribute_value(
                                        details::ccl_api_type_attr_traits<ccl::event_attr_id,
                                        ccl::event_attr_id::native_handle>{});
            native_event.wait();
        #else
            throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
        #endif
        completed = true;
    }
}

bool native_request_impl::test() {
    if (!completed) {
        throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
    }
    return completed;
}

bool native_request_impl::cancel() {
    throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
}

event::native_t& native_request_impl::get_native() {
    return ev->get_attribute_value(
                    details::ccl_api_type_attr_traits<ccl::event_attr_id,
                    ccl::event_attr_id::native_handle>{});
}

} // namespace ccl
