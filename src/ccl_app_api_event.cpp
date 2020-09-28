#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "common/request/request_impl.hpp"
#include "oneapi/ccl/ccl_event.hpp"

namespace ccl {

CCL_API event::event() noexcept : base_t(nullptr) {}
CCL_API event::event(event&& src) noexcept : base_t(std::move(src)) {}
CCL_API event::event(impl_value_t&& impl) noexcept : base_t(std::move(impl)) {}
CCL_API event::~event() noexcept {}

CCL_API event& event::operator=(event&& src) noexcept {
    if (this->get_impl() != src.get_impl()) {
        this->get_impl() = std::move(src.get_impl());
    }
    return *this;
}

bool CCL_API event::operator==(const event& rhs) const noexcept {
    return this->get_impl() == rhs.get_impl();
}

bool CCL_API event::operator!=(const event& rhs) const noexcept {
    return this->get_impl() != rhs.get_impl();
}

void CCL_API event::wait() {
    return get_impl()->wait();
}

bool CCL_API event::test() {
    return get_impl()->test();
}

bool CCL_API event::cancel() {
    return get_impl()->cancel();
}

CCL_API event_internal& event::get_event() {
    return get_impl()->get_event();
}

} // namespace ccl
