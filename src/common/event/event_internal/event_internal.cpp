#include "common/event/event_internal/event_internal_impl.hpp"

#if 0
namespace ccl {

event_internal::event_internal(event_internal&& src) : base_t(std::move(src)) {}

event_internal::event_internal(impl_value_t&& impl) : base_t(std::move(impl)) {}

event_internal::~event_internal() {}

event_internal& event_internal::operator=(event_internal&& src) {
    if (src.get_impl() != this->get_impl()) {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}

void event_internal::build_from_params() {
    get_impl()->build_from_params();
}
} // namespace ccl
#endif
