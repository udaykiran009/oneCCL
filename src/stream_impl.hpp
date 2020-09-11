#pragma once
#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_stream_attr_ids.hpp"
#include "oneapi/ccl/ccl_stream_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_stream.hpp"
#include "common/stream/stream.hpp"

namespace ccl
{
/* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
template <class ...attr_value_pair_t>
stream stream::create_stream_from_attr(typename unified_device_type::ccl_native_t device, attr_value_pair_t&&...avps)
{
    ccl::version ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    stream str{stream_provider_dispatcher::create(device, ret)};
    int expander [] {(str.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
    (void) expander;
    str.build_from_params();
    return str;
}

template <class ...attr_value_pair_t>
stream stream::create_stream_from_attr(typename unified_device_type::ccl_native_t device,
                               typename unified_device_context_type::ccl_native_t context,
                               attr_value_pair_t&&...avps)
{
    ccl::version ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    stream str{stream_provider_dispatcher::create(device, context, ret)};
    int expander [] {(str.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
    (void)expander;
    str.build_from_params();
    return str;
}

template <class native_stream_type,
          typename T>
stream stream::create_stream(native_stream_type& native_stream)
{
    ccl::version ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;
    return {stream_provider_dispatcher::create(native_stream, ret)};
}

template <class device_type, class native_context_type,
           typename T>
stream stream::create_stream(device_type& device, native_context_type& native_ctx)
{
    ccl::version ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;
    return {stream_provider_dispatcher::create(device, native_ctx, ret)};
}


template <stream_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<stream_attr_id, attrId>::return_type& stream::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<stream_attr_id, attrId>{});
}


template<stream_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<stream_attr_id, attrId>::return_type stream::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<stream_attr_id, attrId> {});
}

/*
stream::stream(const typename details::ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::version>::type& version) :
        base_t(stream_provider_dispatcher::create(version))
{
}*/
}


/***************************TypeGenerations*********************************************************/
#define API_STREAM_CREATION_FORCE_INSTANTIATION(native_stream_type)                                 \
template                                                                                            \
CCL_API ccl::stream ccl::stream::create_stream(native_stream_type& native_stream);

#define API_STREAM_CREATION_EXT_FORCE_INSTANTIATION(device_type, native_context_type)               \
template                                                                                            \
CCL_API ccl::stream ccl::stream::create_stream(device_type& device, native_context_type& native_ctx);


#define API_STREAM_FORCE_INSTANTIATION_SET(IN_attrId, IN_Value)                                     \
template                                                                                            \
CCL_API typename ccl::details::ccl_api_type_attr_traits<ccl::stream_attr_id, IN_attrId>::return_type ccl::stream::set<IN_attrId, IN_Value>(const IN_Value& v);

#define API_STREAM_FORCE_INSTANTIATION_GET(IN_attrId)                                  \
template                                                                                            \
CCL_API const typename ccl::details::ccl_api_type_attr_traits<ccl::stream_attr_id, IN_attrId>::return_type& ccl::stream::get<IN_attrId>() const;


#define API_STREAM_FORCE_INSTANTIATION(IN_attrId, IN_Value)                       \
API_STREAM_FORCE_INSTANTIATION_SET(IN_attrId, IN_Value)                           \
API_STREAM_FORCE_INSTANTIATION_GET(IN_attrId)


#endif //#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
