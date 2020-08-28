#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

class ccl_host_comm_split_attr_impl;
class ccl_device_comm_split_attr_impl;
struct ccl_empty_attr;
/**
 * Host attributes
 */
class comm_split_attr_t : public ccl_api_base_copyable<comm_split_attr_t, copy_on_write_access_policy, ccl_host_comm_split_attr_impl>
{
public:
    using base_t = ccl_api_base_copyable<comm_split_attr_t, copy_on_write_access_policy, ccl_host_comm_split_attr_impl>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    comm_split_attr_t& operator=(comm_split_attr_t&& src);
    comm_split_attr_t(comm_split_attr_t&& src);
    comm_split_attr_t(const comm_split_attr_t& src);
    comm_split_attr_t(ccl_empty_attr);
    ~comm_split_attr_t() noexcept;

    /**
     * Set specific value for selft attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <ccl_comm_split_attributes attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <ccl_comm_split_attributes attrId>
    const typename details::ccl_host_split_traits<ccl_comm_split_attributes, attrId>::type& get() const;

    template <ccl_comm_split_attributes attrId>
    bool is_valid() const noexcept;

private:
    friend class environment;
    comm_split_attr_t(const typename details::ccl_host_split_traits<ccl_comm_split_attributes, ccl_comm_split_attributes::version>::type& version);

    /* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
    template <class ...attr_value_pair_t>
    static comm_split_attr_t create_comm_split_attr(attr_value_pair_t&&...avps);
};



#ifdef MULTI_GPU_SUPPORT

/**
 * Device attributes
 */
class device_comm_split_attr_t : public ccl_api_base_copyable<device_comm_split_attr_t, copy_on_write_access_policy, ccl_device_comm_split_attr_impl>
{
public:
    using base_t = ccl_api_base_copyable<device_comm_split_attr_t, copy_on_write_access_policy, ccl_device_comm_split_attr_impl>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    device_comm_split_attr_t& operator=(device_comm_split_attr_t&& src);
    device_comm_split_attr_t(device_comm_split_attr_t&& src);
    device_comm_split_attr_t(const device_comm_split_attr_t& src);
    device_comm_split_attr_t(ccl_empty_attr);
    ~device_comm_split_attr_t() noexcept;

    /**
     * Set specific value for selft attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <ccl_comm_split_attributes attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <ccl_comm_split_attributes attrId>
    const typename details::ccl_device_split_traits<ccl_comm_split_attributes, attrId>::type& get() const;

    template <ccl_comm_split_attributes attrId>
    bool is_valid() const noexcept;

private:
    friend class environment;
    device_comm_split_attr_t(const typename details::ccl_device_split_traits<ccl_comm_split_attributes, ccl_comm_split_attributes::version>::type& version);

    /* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
    template <class ...attr_value_pair_t>
    static device_comm_split_attr_t create_device_comm_split_attr(attr_value_pair_t&&...avps);
};

#endif



template <ccl_comm_split_attributes t, class value_type>
constexpr auto attr_arg(value_type v) -> details::attr_value_tripple<ccl_comm_split_attributes,
                                                              t, value_type>
{
    return details::attr_value_tripple<ccl_comm_split_attributes, t, value_type>(v);
}
} // namespace ccl
