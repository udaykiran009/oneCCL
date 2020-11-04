#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_comm_split_attr_ids.hpp"
#include "oneapi/ccl/ccl_comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_comm_split_attr.hpp"

// Core file with PIMPL implementation
#include "common/comm/comm_split_common_attr.hpp"
#include "comm_split_attr_impl.hpp"

namespace ccl {

namespace v1 {

#define API_FORCE_INSTANTIATION(class_name, IN_attrId, IN_Value, OUT_Traits_Value) \
    template CCL_API IN_Value class_name::set<IN_attrId, IN_Value>(const IN_Value& v); \
\
    template CCL_API const typename OUT_Traits_Value<comm_split_attr_id, IN_attrId>::type& \
    class_name::get<IN_attrId>() const; \
\
    template CCL_API bool class_name::is_valid<IN_attrId>() const noexcept;

/**
 * comm_split_attr attributes definition
 */
CCL_API comm_split_attr::comm_split_attr(ccl_empty_attr)
        : base_t(impl_value_t(new impl_t(ccl_empty_attr::version))) {}
CCL_API comm_split_attr::comm_split_attr(comm_split_attr&& src) : base_t(std::move(src)) {}

CCL_API comm_split_attr::comm_split_attr(const comm_split_attr& src) : base_t(src) {}

CCL_API comm_split_attr::comm_split_attr(
    const typename detail::ccl_api_type_attr_traits<comm_split_attr_id,
                                                    comm_split_attr_id::version>::type& version)
        : base_t(impl_value_t(new impl_t(version))) {}

CCL_API comm_split_attr::~comm_split_attr() noexcept {}

CCL_API comm_split_attr& comm_split_attr::operator=(const comm_split_attr& src) {
    this->get_impl() = src.get_impl();
    return *this;
}

CCL_API comm_split_attr& comm_split_attr::operator=(comm_split_attr&& src) {
    if (src.get_impl() != this->get_impl()) {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}

API_FORCE_INSTANTIATION(comm_split_attr,
                        comm_split_attr_id::color,
                        int,
                        detail::ccl_api_type_attr_traits)
API_FORCE_INSTANTIATION(comm_split_attr,
                        comm_split_attr_id::group,
                        group_split_type,
                        detail::ccl_api_type_attr_traits)
API_FORCE_INSTANTIATION(comm_split_attr,
                        comm_split_attr_id::version,
                        ccl::library_version,
                        detail::ccl_api_type_attr_traits)

#undef API_FORCE_INSTANTIATION

} // namespace v1

} // namespace ccl
