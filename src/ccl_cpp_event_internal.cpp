#include "oneapi/ccl/ccl_types.hpp"
#include "event_internal_impl.hpp"

namespace ccl {

CCL_API event_internal::event_internal(event_internal&& src) : base_t(std::move(src)) {}

CCL_API event_internal::event_internal(impl_value_t&& impl) : base_t(std::move(impl)) {}

CCL_API event_internal::~event_internal() {}

CCL_API event_internal& event_internal::operator=(event_internal&& src) {
    if (src.get_impl() != this->get_impl()) {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}

CCL_API void event_internal::build_from_params() {
    get_impl()->build_from_params();
}
} // namespace ccl

/*
API_EVENT_CREATION_FORCE_INSTANTIATION(typename ccl::unified_event_type::ccl_native_t);
#ifdef CCL_ENABLE_SYCL
    API_EVENT_CREATION_EXT_FORCE_INSTANTIATION(cl_event)
#endif


API_EVENT_FORCE_INSTANTIATION(ccl::event_attr_id::version, ccl::library_version);
API_EVENT_FORCE_INSTANTIATION_GET(ccl::event_attr_id::native_handle);
API_EVENT_FORCE_INSTANTIATION_GET(ccl::event_attr_id::context)
API_EVENT_FORCE_INSTANTIATION(ccl::event_attr_id::command_type, uint32_t);
API_EVENT_FORCE_INSTANTIATION(ccl::event_attr_id::command_execution_status, int64_t);
*/
