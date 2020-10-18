#pragma once

#if 0

#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"

#include "common/event/ccl_event_attr_ids.hpp"
#include "common/event/ccl_event_attr_ids_traits.hpp"
#include "common/event/event_internal/event_internal.hpp"

#include "common/event/ccl_event.hpp"
#include "common/utils/version.hpp"

namespace ccl {

template <class event_type, class... attr_value_pair_t>
event_internal event_internal::create_event_from_attr(event_type& native_event_handle,
                                    typename unified_context_type::ccl_native_t context,
                                    attr_value_pair_t&&... avps) {
    auto version = utils::get_library_version();

    event_internal str{ event_internal::impl_value_t(new event_internal::impl_t(native_event_handle, context, version)) };
    int expander[]{ (str.template set<attr_value_pair_t::idx()>(avps.val()), 0)... };
    (void)expander;
    str.build_from_params();

    return str;
}

template <class event_handle_type, typename T>
event_internal event_internal::create_event(event_handle_type native_event_handle,
                          typename unified_context_type::ccl_native_t context) {
    auto version = utils::get_library_version();

    event_internal str{ event_internal::impl_value_t(new event_internal::impl_t(native_event_handle, context, version)) };
    str.build_from_params();

    return str;
}

template <class event_type, typename T>
event_internal event_internal::create_event(event_type& native_event) {
    auto version = utils::get_library_version();

    return { event_internal::impl_value_t(new event_internal::impl_t(native_event, version)) };
}

template <event_attr_id attrId>
const typename detail::ccl_api_type_attr_traits<event_attr_id, attrId>::return_type&
event_internal::get() const {
    return get_impl()->get_attribute_value(
        detail::ccl_api_type_attr_traits<event_attr_id, attrId>{});
}

template<event_attr_id attrId,
             class Value/*,
             typename T*/>
typename ccl::detail::ccl_api_type_attr_traits<ccl::event_attr_id, attrId>::return_type event_internal::set(const Value& v)
{
    return get_impl()->set_attribute_value(
        v, detail::ccl_api_type_attr_traits<event_attr_id, attrId>{});
}

} // namespace ccl

#endif
