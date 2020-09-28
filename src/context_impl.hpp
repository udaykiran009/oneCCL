#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"

#include "oneapi/ccl/ccl_context_attr_ids.hpp"
#include "oneapi/ccl/ccl_context_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_context.hpp"

#include "common/context/context.hpp"
namespace ccl {

template <class device_context_type, class... attr_value_pair_t>
CCL_API context context::create_context_from_attr(device_context_type& native_device_context_handle,
                                       attr_value_pair_t&&... avps) {
    ccl::library_version ret{};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    context str{ context::impl_value_t(new context::impl_t(native_device_context_handle, ret)) };
    int expander[]{ (str.template set<attr_value_pair_t::idx()>(avps.val()), 0)... };
    (void)expander;
    str.build_from_params();

    return str;
}

template <class device_context_type, typename T>
CCL_API context context::create_context(device_context_type&& native_device_context) {
    ccl::library_version ret{};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    return { context::impl_value_t(new context::impl_t(std::forward<device_context_type>(native_device_context), ret)) };
}

template <context_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<context_attr_id, attrId>::return_type&
context::get() const {
    return get_impl()->get_attribute_value(
        details::ccl_api_type_attr_traits<context_attr_id, attrId>{});
}

template<context_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename ccl::details::ccl_api_type_attr_traits<ccl::context_attr_id, attrId>::return_type context::set(const Value& v)
{
    return get_impl()->set_attribute_value(
        v, details::ccl_api_type_attr_traits<context_attr_id, attrId>{});
}

} // namespace ccl

/***************************TypeGenerations*********************************************************/
#define API_DEVICE_CONTEXT_CREATION_FORCE_INSTANTIATION(native_device_context_type) \
    template CCL_API ccl::context ccl::context::create_context(native_device_context_type&& ctx);   \
    template CCL_API ccl::context ccl::context::create_context(native_device_context_type& ctx);    \

#define API_DEVICE_CONTEXT_FORCE_INSTANTIATION_SET(IN_attrId, IN_Value) \
    template CCL_API typename ccl::details::ccl_api_type_attr_traits<ccl::context_attr_id, \
                                                                     IN_attrId>::return_type \
    ccl::context::set<IN_attrId, IN_Value>(const IN_Value& v);

#define API_DEVICE_CONTEXT_FORCE_INSTANTIATION_GET(IN_attrId) \
    template CCL_API const typename ccl::details:: \
        ccl_api_type_attr_traits<ccl::context_attr_id, IN_attrId>::return_type& \
        ccl::context::get<IN_attrId>() const;

#define API_DEVICE_CONTEXT_FORCE_INSTANTIATION(IN_attrId, IN_Value) \
    API_DEVICE_CONTEXT_FORCE_INSTANTIATION_SET(IN_attrId, IN_Value) \
    API_DEVICE_CONTEXT_FORCE_INSTANTIATION_GET(IN_attrId)
