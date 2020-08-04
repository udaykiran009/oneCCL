#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

class ccl_stream;
namespace ccl {

/**
 * A stream object is an abstraction over CPU/GPU streams
 * Has no defined public constructor. Use ccl::environment::create_stream
 * for stream objects creation
 */
/**
 * Stream class
 */
class stream : public ccl_api_base_movable<stream, direct_access_policy, ccl_stream>
{
public:
    using base_t = ccl_api_base_movable<stream, direct_access_policy, ccl_stream>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    ~stream();

    /**
     * Get specific attribute value by @attrId
     */
    template <stream_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<stream_attr_id, attrId>::return_type& get() const;

private:
    friend class environment;
    friend class communicator;
    friend class device_communicator;
    stream(stream&& src);
    stream(impl_value_t&& impl);

    /**
     *Parametrized stream creation helper
     */
    template <stream_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set(const Value& v);

    void build_from_params();
    stream(const typename details::ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::version>::type& version);
};


template <stream_attr_id t, class value_type>
constexpr auto attr_arg(value_type v) -> details::attr_value_tripple<stream_attr_id, t, value_type>
{
    return details::attr_value_tripple<stream_attr_id, t, value_type>(v);
}

/* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
template <class native_stream_type,
          class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type>
stream create_stream(native_stream_type& native_stream);

template <class ...attr_value_pair_t>
stream create_stream_from_attr(typename unified_device_type::native_reference_t device, attr_value_pair_t&&...avps);

template <class ...attr_value_pair_t>
stream create_stream_from_attr(typename unified_device_type::native_reference_t device,
                               typename unified_device_context_type::native_reference_t context,
                               attr_value_pair_t&&...avps);

} // namespace ccl
