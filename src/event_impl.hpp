#pragma once
#include "ccl_types.hpp"
#include "ccl_event.hpp"
#include "common/event/event.hpp"

namespace ccl
{
/* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
template <class event_type,
          class ...attr_value_pair_t>
event event::create_event_from_attr(event_type& native_event_handle,
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

    event str {event::impl_value_t(new event::impl_t(native_event_handle, context, ret))};
    int expander [] {(str.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
    (void)expander;
    str.build_from_params();

    return str;
}

CCL_API event::event(event&& src) :
        base_t(std::move(src))
{
}

CCL_API event::event(impl_value_t&& impl) :
        base_t(std::move(impl))
{
}

CCL_API event::~event()
{
}

CCL_API event& event::operator=(event&& src)
{
    if (src.get_impl() != this->get_impl())
    {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}

template <event_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<event_attr_id, attrId>::return_type& event::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<event_attr_id, attrId>{});
}


template<event_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value event::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<event_attr_id, attrId> {});
}

void event::build_from_params()
{
    get_impl()->build_from_params();
}
}
