#include "coll/ccl_bcast_op_attr.hpp"

namespace ccl {

ccl_bcast_op_attr_impl_t::ccl_bcast_op_attr_impl_t(const typename ccl_common_op_attr_impl_t::version_traits_t::type& version) :
        base_t(version)
{
}
}
