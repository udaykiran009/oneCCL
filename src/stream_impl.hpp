#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_stream_attr_ids.hpp"
#include "oneapi/ccl/ccl_stream_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_stream.hpp"
#include "common/stream/stream.hpp"
#include "common/utils/version.hpp"

namespace ccl {

namespace v1 {

/* TODO temporary function for UT compilation: would be part of ccl::detail::environment in final*/
template <class... attr_value_pair_t>
stream stream::create_stream_from_attr(typename unified_device_type::ccl_native_t device,
                                       attr_value_pair_t&&... avps) {
    auto version = utils::get_library_version();

    stream str{ stream_provider_dispatcher::create(device, version) };
    int expander[]{ (str.template set<attr_value_pair_t::idx()>(avps.val()), 0)... };
    (void)expander;
    str.build_from_params();
    return str;
}

template <class... attr_value_pair_t>
stream stream::create_stream_from_attr(typename unified_device_type::ccl_native_t device,
                                       typename unified_context_type::ccl_native_t context,
                                       attr_value_pair_t&&... avps) {
    auto version = utils::get_library_version();

    stream str{ stream_provider_dispatcher::create(device, context, version) };
    int expander[]{ (str.template set<attr_value_pair_t::idx()>(avps.val()), 0)... };
    (void)expander;
    str.build_from_params();
    return str;
}

template <class native_stream_type, typename T>
stream stream::create_stream(native_stream_type& native_stream) {
    auto version = utils::get_library_version();
    return { stream_provider_dispatcher::create(native_stream, version) };
}

template <class device_type, class native_context_type, typename T>
stream stream::create_stream(device_type& device, native_context_type& native_ctx) {
    auto version = utils::get_library_version();
    return { stream_provider_dispatcher::create(device, native_ctx, version) };
}

template <stream_attr_id attrId>
CCL_API const typename detail::ccl_api_type_attr_traits<stream_attr_id, attrId>::return_type&
stream::get() const {
    return get_impl()->get_attribute_value(
        detail::ccl_api_type_attr_traits<stream_attr_id, attrId>{});
}

template<stream_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename detail::ccl_api_type_attr_traits<stream_attr_id, attrId>::return_type stream::set(const Value& v)
{
    return get_impl()->set_attribute_value(
        v, detail::ccl_api_type_attr_traits<stream_attr_id, attrId>{});
}

/*
stream::stream(const typename detail::ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::version>::type& version) :
        base_t(stream_provider_dispatcher::create(version))
{
}*/

} // namespace v1

} // namespace ccl

/***************************TypeGenerations*********************************************************/
#define API_STREAM_CREATION_FORCE_INSTANTIATION(native_stream_type) \
    template CCL_API ccl::stream ccl::stream::create_stream(native_stream_type& native_stream);

#define API_STREAM_CREATION_EXT_FORCE_INSTANTIATION(device_type, native_context_type) \
    template CCL_API ccl::stream ccl::stream::create_stream(device_type& device, \
                                                            native_context_type& native_ctx);

#define API_STREAM_FORCE_INSTANTIATION_SET(IN_attrId, IN_Value) \
    template CCL_API typename ccl::detail::ccl_api_type_attr_traits<ccl::stream_attr_id, \
                                                                    IN_attrId>::return_type \
    ccl::stream::set<IN_attrId, IN_Value>(const IN_Value& v);

#define API_STREAM_FORCE_INSTANTIATION_GET(IN_attrId) \
    template CCL_API const typename ccl::detail::ccl_api_type_attr_traits<ccl::stream_attr_id, \
                                                                          IN_attrId>::return_type& \
    ccl::stream::get<IN_attrId>() const;

#define API_STREAM_FORCE_INSTANTIATION(IN_attrId, IN_Value) \
    API_STREAM_FORCE_INSTANTIATION_SET(IN_attrId, IN_Value) \
    API_STREAM_FORCE_INSTANTIATION_GET(IN_attrId)
