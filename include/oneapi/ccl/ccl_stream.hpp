#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)

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
class stream : public ccl_api_base_copyable<stream, direct_access_policy, ccl_stream>
{
public:
    using base_t = ccl_api_base_copyable<stream, direct_access_policy, ccl_stream>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    ~stream();

    stream(stream&& src);
    stream(const stream& src);
    stream& operator= (const stream&src);

    /**
     * Get specific attribute value by @attrId
     */
    template <stream_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<stream_attr_id, attrId>::return_type& get() const;

private:
    friend class environment;
    friend class communicator;
    friend class device_communicator;
    friend struct ccl_empty_attr;

    template <class ...attr_value_pair_t>
    friend stream create_stream_from_attr(typename unified_device_type::ccl_native_t device,
                               typename unified_device_context_type::ccl_native_t context,
                               attr_value_pair_t&&...avps);
    template <class ...attr_value_pair_t>
    friend stream create_stream_from_attr(typename unified_device_type::ccl_native_t device, attr_value_pair_t&&...avps);

    stream(impl_value_t&& impl);

    /**
     *Parametrized stream creation helper
     */
    template <stream_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    typename details::ccl_api_type_attr_traits<stream_attr_id, attrId>::return_type set(const Value& v);

    void build_from_params();
    stream(const typename details::ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::version>::type& version);

    /**
     *  Factory methods
     */
    template <class native_stream_type,
          class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type>
    static stream create_stream(native_stream_type& native_stream);

    template <class native_stream_type, class native_context_type,
          class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type>
    static stream create_stream(native_stream_type& native_stream, native_context_type& native_ctx);

    template <class ...attr_value_pair_t>
    static stream create_stream_from_attr(typename unified_device_type::ccl_native_t device, attr_value_pair_t&&...avps);

    template <class ...attr_value_pair_t>
    static stream create_stream_from_attr(typename unified_device_type::ccl_native_t device,
                               typename unified_device_context_type::ccl_native_t context,
                               attr_value_pair_t&&...avps);
};


template <stream_attr_id t, class value_type>
constexpr auto attr_arg(value_type v) -> details::attr_value_tripple<stream_attr_id, t, value_type>
{
    return details::attr_value_tripple<stream_attr_id, t, value_type>(v);
}


/**
 * Declare extern empty attributes
 */
extern stream default_stream;



#endif //#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
} // namespace ccl
