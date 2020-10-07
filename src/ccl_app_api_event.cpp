#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "common/event/impls/event_impl.hpp"
#include "common/event/impls/empty_event.hpp"
#include "common/event/impls/native_event.hpp"

namespace ccl {

CCL_API event::event() noexcept : base_t(impl_value_t(new empty_event_impl())) {}
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

CCL_API event::operator bool() {
    return this->test();
}

void CCL_API event::wait() {
    get_impl()->wait();
}

bool CCL_API event::test() {
    return get_impl()->test();
}

bool CCL_API event::cancel() {
    return get_impl()->cancel();
}

CCL_API event::native_t& event::get_native() {
    return const_cast<event::native_t&>(get_impl()->get_native());
}

CCL_API const event::native_t& event::get_native() const {
    return get_impl()->get_native();
}

event CCL_API event::create_from_native(native_t& native_event) {
    library_version version;
    version.major = CCL_MAJOR_VERSION;
    version.minor = CCL_MINOR_VERSION;
    version.update = CCL_UPDATE_VERSION;
    version.product_status = CCL_PRODUCT_STATUS;
    version.build_date = CCL_PRODUCT_BUILD_DATE;
    version.full = CCL_PRODUCT_FULL;

    return impl_value_t(
        new native_event_impl(native_event, version)
    );
}

} // namespace ccl
