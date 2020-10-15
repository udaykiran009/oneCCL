#include "common/event/impls/native_event.hpp"
#include "common/log/log.hpp"

namespace ccl {

native_event_impl::native_event_impl(std::unique_ptr<ccl_event> ev)
    : ev(std::move(ev)) {
}

void native_event_impl::wait() {
    if (!completed) {
        #ifdef CCL_ENABLE_SYCL
            auto native_event = ev->get_attribute_value(
                                        detail::ccl_api_type_attr_traits<ccl::event_attr_id,
                                        ccl::event_attr_id::native_handle>{});
            native_event.wait();
        #else
            throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
        #endif
        completed = true;
    }
}

bool native_event_impl::test() {
    if (!completed) {
        throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
    }
    return completed;
}

bool native_event_impl::cancel() {
    throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
}

event::native_t& native_event_impl::get_native() {
    return ev->get_attribute_value(
                    detail::ccl_api_type_attr_traits<ccl::event_attr_id,
                    ccl::event_attr_id::native_handle>{});
}

} // namespace ccl
