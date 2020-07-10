#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

/**
 * Common operation attributes id
 */
enum class common_op_attr_id : int {
    match_id,

    last_value
};

/**
 *  traits for attributes
 */
template <common_op_attr_id attrId>
struct common_op_attr_traits {};

/* TODO other traits for common op*/

/**
 * Traits attributes specializations
 */
template <>
struct common_op_attr_traits<common_op_attr_id::match_id>
{
    using type = const char *;
};


/**
 * Collective attributes
 */
enum class allgather_op_attr_id : int
{
    op_id_offset = common_op_attr_id::last_value,
    prolog_fn_id = op_id_offset,

    last_value
};


/* TODO other collective ops attributes id*/


/**
 * Traits for allgather attributes
 */
template <allgather_op_attr_id attrId>
struct allgatherv_attr_traits : public common_op_attr_traits {};

/**
 * Specialization for allgather op attributes
 */
template <>
struct allgatherv_attr_traits<allgather_op_attr_id::prolog_fn_id> {
    using type = std::variant<std::function<bool(char, char)>,
                              std::function<bool(int, int)>,
                              std::function<bool(double, double)>>;    //or use #define for types supporting
};

/**
 * Allgather coll attributes
 */

struct ccl_allgather_op_attr
        : public pointer_on_impl<ccl_allgather_op_attr, ccl_allgather_impl_t> {

    using impl_value_t = typename pointer_on_impl<ccl_allgather_op_attr,
                                                  ccl_allgather_impl_t>::impl_value_t;
    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <allgather_op_attr_id attrId,
              class Value,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <allgather_op_attr_id attrId>
    const typename allgatherv_attr_traits<attrId>::type& get_value() const;

private:
    ccl_allgather_op_attr(impl_value_t&& impl);
};

/* TODO Similar coll attributes*/

}
