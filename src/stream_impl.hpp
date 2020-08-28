#pragma once
#include "ccl_types.hpp"
#include "ccl_stream.hpp"
#include "common/stream/stream.hpp"

namespace ccl
{
/* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
template <class ...attr_value_pair_t>
stream stream::create_stream_from_attr(typename unified_device_type::ccl_native_t device, attr_value_pair_t&&...avps)
{
    ccl_version_t ret {};
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
    ccl_version_t ret {};
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

CCL_API stream::stream(const typename details::ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::version>::type& version):
        base_t(impl_value_t())
{
}

CCL_API stream::stream(stream&& src) :
        base_t(std::move(src))
{
}

CCL_API stream::stream(impl_value_t&& impl) :
        base_t(std::move(impl))
{
}

CCL_API stream::~stream()
{
}

CCL_API stream::stream(const stream& src) :
        base_t(src)
{
}

CCL_API stream& stream::operator= (const stream&src)
{
    if (src.get_impl() != this->get_impl())
    {
        this->get_impl() = src.get_impl();
    }
    return *this;
}

template <stream_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<stream_attr_id, attrId>::return_type& stream::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<stream_attr_id, attrId>{});
}


template<stream_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value stream::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<stream_attr_id, attrId> {});
}

void stream::build_from_params()
{
    get_impl()->build_from_params();
}
/*
stream::stream(const typename details::ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::version>::type& version) :
        base_t(stream_provider_dispatcher::create(version))
{
}*/
}
