#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl
{

class ccl_datatype_attr_impl;

class datatype_attr : public ccl_api_base_copyable<datatype_attr, copy_on_write_access_policy, ccl_datatype_attr_impl>
{
public:
    using base_t = ccl_api_base_copyable<datatype_attr, copy_on_write_access_policy, ccl_datatype_attr_impl>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    datatype_attr& operator=(const datatype_attr& src);
    datatype_attr& operator=(datatype_attr&& src);
    datatype_attr(datatype_attr&& src);
    datatype_attr(const datatype_attr& src);
    ~datatype_attr() noexcept;

    /**
     * Set specific value for selft attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <ccl_datatype_attributes attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::return_type*/>
    Value set(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <ccl_datatype_attributes attrId>
    const typename details::ccl_api_type_attr_traits<ccl_datatype_attributes, attrId>::return_type& get() const;

private:
    friend class environment;
    datatype_attr(const typename details::ccl_api_type_attr_traits<ccl_datatype_attributes, ccl_datatype_attributes::version>::return_type& version);
};

template <ccl_datatype_attributes t, class value_type>
constexpr auto attr_arg(value_type v) -> details::attr_value_tripple<ccl_datatype_attributes,
                                                              t, value_type>
{
    return details::attr_value_tripple<ccl_datatype_attributes, t, value_type>(v);
}

} // namespace ccl
