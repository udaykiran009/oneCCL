#include "coll/ccl_alltoallv_op_attr.hpp"
namespace ccl {

ccl_alltoallv_attr_impl_t::ccl_alltoallv_attr_impl_t(const typename ccl_operation_attr_impl_t::version_traits_t::type& version) :
        base_t(version)
{
}
}
