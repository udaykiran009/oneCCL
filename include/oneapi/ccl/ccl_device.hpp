#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

class ccl_device_impl;
namespace ccl {

/**
 * A device object is an abstraction over CPU/GPU device
 * Has no defined public constructor. Use ccl::environment::create_device
 * for device objects creation
 */
/**
 * Stream class
 */
class device : public ccl_api_base_movable<device, direct_access_policy, ccl_device_impl> {
public:
    using base_t = ccl_api_base_movable<device, direct_access_policy, ccl_device_impl>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    /**
     * Declare native device type
     */
    using native_t = typename details::ccl_api_type_attr_traits<ccl::device_attr_id,
                                                                ccl::device_attr_id::native_handle>::return_type;

    device(device&& src);
    device& operator=(device&& src);
    ~device();

    /**
     * Get specific attribute value by @attrId
     */
    template <device_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<device_attr_id, attrId>::return_type& get()
        const;

    /**
     * Get native device object
     */
     native_t& get_native();
     const native_t& get_native() const;
private:
    friend class environment;
    friend class communicator;
    device(impl_value_t&& impl);

    /**
     *Parametrized device creation helper
     */
    template <device_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    typename ccl::details::ccl_api_type_attr_traits<ccl::device_attr_id, attrId>::return_type set(const Value& v);

    void build_from_params();
    device(const typename details::ccl_api_type_attr_traits<device_attr_id,
                                                           device_attr_id::version>::type& version);

    /**
     * Factory methods
     */
    template <class device_type,
              class = typename std::enable_if<is_device_supported<device_type>()>::type>
    static device create_device(device_type&& native_device);

    template <class device_handle_type, class... attr_value_pair_t>
    static device create_device_from_attr(device_handle_type& native_device_handle,
                                        attr_value_pair_t&&... avps);
};

template <device_attr_id t, class value_type>
constexpr auto attr_val(value_type v) -> details::attr_value_tripple<device_attr_id, t, value_type> {
    return details::attr_value_tripple<device_attr_id, t, value_type>(v);
}

} // namespace ccl
