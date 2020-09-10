#include "coll/ccl_allgather_op_attr.hpp"

namespace ccl {

ccl_allgatherv_attr_impl_t::ccl_allgatherv_attr_impl_t (const base_t& base) :
        base_t(base)
{
}
ccl_allgatherv_attr_impl_t::ccl_allgatherv_attr_impl_t(const typename ccl_common_op_attr_impl_t::version_traits_t::type& version) :
        base_t(version)
{
}
ccl_allgatherv_attr_impl_t::ccl_allgatherv_attr_impl_t(const ccl_allgatherv_attr_impl_t& src) :
        base_t(src)
{
}

typename ccl_allgatherv_attr_impl_t::vector_buf_traits_t::type
ccl_allgatherv_attr_impl_t::set_attribute_value(typename vector_buf_traits_t::type val, const vector_buf_traits_t&t)
{
    auto old = vector_buf_id_val;
    std::swap(vector_buf_id_val, val);
    return old;
}

const typename ccl_allgatherv_attr_impl_t::vector_buf_traits_t::type&
ccl_allgatherv_attr_impl_t::get_attribute_value(const vector_buf_traits_t& id) const
{
    return vector_buf_id_val;
}
}
