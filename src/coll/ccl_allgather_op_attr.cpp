#include "coll/ccl_allgather_op_attr.hpp"

namespace ccl {

ccl_allgatherv_attr_impl_t::ccl_allgatherv_attr_impl_t(const base_t& base) : base_t(base) {}
ccl_allgatherv_attr_impl_t::ccl_allgatherv_attr_impl_t(
    const typename ccl_operation_attr_impl_t::version_traits_t::type& version)
        : base_t(version) {}
ccl_allgatherv_attr_impl_t::ccl_allgatherv_attr_impl_t(const ccl_allgatherv_attr_impl_t& src)
        : base_t(src) {}
} // namespace ccl
