#include "ccl_types.h"
#include "ccl_aliases.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_datatype_attr_ids.hpp"
#include "ccl_datatype_attr_ids_traits.hpp"
#include "ccl_datatype_attr.hpp"

// Core file with PIMPL implementation
#include "common/datatype/datatype_attr.hpp"
#include "datatype_attr_impl.hpp"

namespace ccl
{

#define COMMA   ,
#define API_FORCE_SETTER_INSTANTIATION(class_name, IN_attrId, IN_Value, OUT_Traits_Value)           \
template                                                                                            \
CCL_API IN_Value class_name::set<IN_attrId, IN_Value>(const IN_Value& v);

#define API_FORCE_GETTER_INSTANTIATION(class_name, IN_attrId, IN_Value, OUT_Traits_Value)           \
template                                                                                            \
CCL_API const typename details::OUT_Traits_Value<ccl_datatype_attributes, IN_attrId>::return_type& class_name::get<IN_attrId>() const;

/**
 * datatype_attr_t attributes definition
 */
CCL_API datatype_attr_t::datatype_attr_t(datatype_attr_t&& src) :
        base_t(std::move(src))
{
}

CCL_API datatype_attr_t::datatype_attr_t(const datatype_attr_t& src) :
        base_t(src)
{
}

CCL_API datatype_attr_t::datatype_attr_t(
        const typename details::ccl_api_type_attr_traits<ccl_datatype_attributes,
        ccl_datatype_attributes::version>::return_type& version) :
        base_t(std::shared_ptr<impl_t>(new impl_t(version)))
{
}

CCL_API datatype_attr_t::~datatype_attr_t() noexcept
{
}

CCL_API datatype_attr_t& datatype_attr_t::operator=(const datatype_attr_t& src)
{
    this->get_impl() = src.get_impl();
    return *this;
}

CCL_API datatype_attr_t& datatype_attr_t::operator=(datatype_attr_t&& src)
{
    if (src.get_impl() != this->get_impl())
    {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}

API_FORCE_SETTER_INSTANTIATION(datatype_attr_t, ccl_datatype_attributes::size, int, ccl_api_type_attr_traits);
API_FORCE_SETTER_INSTANTIATION(datatype_attr_t, ccl_datatype_attributes::size, size_t, ccl_api_type_attr_traits);
API_FORCE_GETTER_INSTANTIATION(datatype_attr_t, ccl_datatype_attributes::size, size_t, ccl_api_type_attr_traits);
API_FORCE_GETTER_INSTANTIATION(datatype_attr_t, ccl_datatype_attributes::version, ccl_version_t, ccl_api_type_attr_traits);

#undef API_FORCE_SETTER_INSTANTIATION
#undef API_FORCE_GETTER_INSTANTIATION
#undef COMMA

} // namespace ccl
