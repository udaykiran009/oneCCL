#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_comm_attr_ids.hpp"
#include "oneapi/ccl/ccl_comm_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_comm_attr.hpp"

// Core file with PIMPL implementation
#include "common/comm/comm_common_attr.hpp"
#include "comm_attr_impl.hpp"

namespace ccl {

#define API_FORCE_INSTANTIATION(class_name, IN_attrId, IN_Value, OUT_Traits_Value) \
    template CCL_API IN_Value class_name::set<IN_attrId, IN_Value>(const IN_Value& v); \
\
    template CCL_API const typename detail::OUT_Traits_Value<comm_attr_id, \
                                                              IN_attrId>::type& \
    class_name::get<IN_attrId>() const; \
\
    template CCL_API bool class_name::is_valid<IN_attrId>() const noexcept;

/**
 * comm_attr attributes definition
 */
CCL_API comm_attr::comm_attr(ccl_empty_attr)
    : base_t(std::shared_ptr<impl_t>(new impl_t(ccl_empty_attr::version))) {}
CCL_API comm_attr::comm_attr(comm_attr&& src)
    : base_t(std::move(src)) {}

CCL_API comm_attr::comm_attr(const comm_attr& src)
    : base_t(src) {}

CCL_API comm_attr::comm_attr(
    const typename detail::ccl_api_type_attr_traits<comm_attr_id,
        comm_attr_id::version>::return_type& version)
    : base_t(std::shared_ptr<impl_t>(new impl_t(version))) {}

CCL_API comm_attr::~comm_attr() noexcept {}

CCL_API comm_attr& comm_attr::operator=(const comm_attr& src) {
    this->get_impl() = src.get_impl();
    return *this;
}

CCL_API comm_attr& comm_attr::operator=(comm_attr&& src) {
    if (src.get_impl() != this->get_impl()) {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}

API_FORCE_INSTANTIATION(comm_attr,
                        comm_attr_id::version,
                        ccl::library_version,
                        ccl_api_type_attr_traits)

#undef API_FORCE_INSTANTIATION

} // namespace ccl
