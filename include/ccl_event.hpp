#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

class ccl_event;
namespace ccl {

/**
 * A event object is an abstraction over CPU/GPU events
 * Has no defined public constructor. Use ccl::environment::create_event
 * for event objects creation
 */
/**
 * Stream class
 */
class event : public ccl_api_base_movable<event, direct_access_policy, ccl_event>
{
public:
    using base_t = ccl_api_base_movable<event, direct_access_policy, ccl_event>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    ~event();

    /**
     * Get specific attribute value by @attrId
     */
    template <event_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<event_attr_id, attrId>::return_type& get() const;

private:
    friend class environment;
    friend class communicator;
    friend class device_communicator;
    event(event&& src);
    event(impl_value_t&& impl);

    /**
     *Parametrized event creation helper
     */
    template <event_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set(const Value& v);

    void build_from_params();
    event(const typename details::ccl_api_type_attr_traits<event_attr_id, event_attr_id::version>::type& version);
};


template <event_attr_id t, class value_type>
constexpr auto attr_arg(value_type v) -> details::attr_value_tripple<event_attr_id, t, value_type>
{
    return details::attr_value_tripple<event_attr_id, t, value_type>(v);
}

/* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
template <class event_type,
          class = typename std::enable_if<is_event_supported<event_type>()>::type>
event create_event(event_type& native_event);

template <class event_type,
          class ...attr_value_pair_t>
event create_event_from_attr(event_type& native_event_handle,
                             typename unified_device_context_type::ccl_native_t context,
                             attr_value_pair_t&&...avps);

} // namespace ccl
