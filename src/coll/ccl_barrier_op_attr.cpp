#include "coll/ccl_barrier_attr.hpp"

namespace ccl {
/**
 * Definition
 */
ccl_barrier_attr_impl_t::ccl_barrier_attr_impl_t(const typename ccl_common_op_attr_impl_t::version_traits_t::type& version) :
        base_t(version)
{
}
}
