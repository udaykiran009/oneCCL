#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_init_attr_ids.hpp"
#include "oneapi/ccl/ccl_init_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_init_attr.hpp"

// Core file with PIMPL implementation
#include "init_attr_impl.hpp"

namespace ccl {

namespace v1 {

#define API_FORCE_SETTER_INSTANTIATION(class_name, IN_attrId, IN_Value, OUT_Traits_Value) \
    template CCL_API IN_Value class_name::set<IN_attrId, IN_Value>(const IN_Value& v);

#define API_FORCE_GETTER_INSTANTIATION(class_name, IN_attrId, OUT_Traits_Value) \
    template CCL_API const typename OUT_Traits_Value<init_attr_id, \
                                                             IN_attrId>::return_type& \
    class_name::get<IN_attrId>() const;

/**
 * init_attr attributes definition
 */
CCL_API init_attr::init_attr(init_attr&& src) : base_t(std::move(src)) {}

CCL_API init_attr::init_attr(const init_attr& src) : base_t(src) {}

CCL_API init_attr::init_attr(
    const typename detail::ccl_api_type_attr_traits<init_attr_id,
                                                    init_attr_id::version>::return_type&
        version)
        : base_t(impl_value_t(new impl_t(version))) {}

CCL_API init_attr::~init_attr() noexcept {}

CCL_API init_attr& init_attr::operator=(const init_attr& src) {
    this->get_impl() = src.get_impl();
    return *this;
}

CCL_API init_attr& init_attr::operator=(init_attr&& src) {
    if (src.get_impl() != this->get_impl()) {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}

API_FORCE_GETTER_INSTANTIATION(init_attr,
                               init_attr_id::version,
                               detail::ccl_api_type_attr_traits);

#undef API_FORCE_SETTER_INSTANTIATION
#undef API_FORCE_GETTER_INSTANTIATION

} // namespace v1

} // namespace ccl
