#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_datatype_attr_ids.hpp"
#include "oneapi/ccl/ccl_datatype_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_datatype_attr.hpp"

// Core file with PIMPL implementation
#include "common/datatype/datatype_attr.hpp"
#include "datatype_attr_impl.hpp"

namespace ccl {

namespace v1 {

#define API_FORCE_SETTER_INSTANTIATION(class_name, IN_attrId, IN_Value, OUT_Traits_Value) \
    template CCL_API IN_Value class_name::set<IN_attrId, IN_Value>(const IN_Value& v);

#define API_FORCE_GETTER_INSTANTIATION(class_name, IN_attrId, OUT_Traits_Value) \
    template CCL_API const typename OUT_Traits_Value<datatype_attr_id, \
                                                              IN_attrId>::return_type& \
    class_name::get<IN_attrId>() const;

/**
 * datatype_attr attributes definition
 */
CCL_API datatype_attr::datatype_attr(datatype_attr&& src) : base_t(std::move(src)) {}

CCL_API datatype_attr::datatype_attr(const datatype_attr& src) : base_t(src) {}

CCL_API datatype_attr::datatype_attr(
    const typename detail::ccl_api_type_attr_traits<datatype_attr_id,
                                                     datatype_attr_id::version>::return_type&
        version)
        : base_t(impl_value_t(new impl_t(version))) {}

CCL_API datatype_attr::~datatype_attr() noexcept {}

CCL_API datatype_attr& datatype_attr::operator=(const datatype_attr& src) {
    this->get_impl() = src.get_impl();
    return *this;
}

CCL_API datatype_attr& datatype_attr::operator=(datatype_attr&& src) {
    if (src.get_impl() != this->get_impl()) {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}

API_FORCE_SETTER_INSTANTIATION(datatype_attr,
                               datatype_attr_id::size,
                               int,
                               detail::ccl_api_type_attr_traits);
API_FORCE_SETTER_INSTANTIATION(datatype_attr,
                               datatype_attr_id::size,
                               size_t,
                               detail::ccl_api_type_attr_traits);
API_FORCE_GETTER_INSTANTIATION(datatype_attr,
                               datatype_attr_id::size,
                               detail::ccl_api_type_attr_traits);
API_FORCE_GETTER_INSTANTIATION(datatype_attr,
                               datatype_attr_id::version,
                               detail::ccl_api_type_attr_traits);

#undef API_FORCE_SETTER_INSTANTIATION
#undef API_FORCE_GETTER_INSTANTIATION

} // namespace v1

} // namespace ccl
