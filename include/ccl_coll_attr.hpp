#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {
template <ccl_coll_type type>
struct ccl_operation_attr_t {};

/**
 * Specializations of operation_attr_t
 */
enum class allgatherv_attr_id : int {
    match_id,
};

/**
 *  traits for attributes
 */
template <allgatherv_attr_id attrId>
struct allgatherv_attr_traits {};

/**
 * Specialization for attributes
 */
template <>
struct allgatherv_attr_traits<allgatherv_attr_id::match_id> {
    using type = const char*;
};

/**
 * Allgather coll attributes
 */

template <>
struct ccl_operation_attr_t<ccl_coll_allgatherv>
        : public pointer_on_impl<ccl_operation_attr_t<ccl_coll_allgatherv>, allgatherv_attr_t> {
    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <allgatherv_attr_id attrId,
              class Value,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <allgatherv_attr_id attrId>
    const typename allgatherv_attr_traits<attrId>::type& get_value() const;

private:
    ccl_operation_attr_t(impl_value_t&& impl);
};

/**
 * TODO Similar coll attributes
 * /

}
