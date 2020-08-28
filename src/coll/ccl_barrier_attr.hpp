#pragma once
#include "ccl_types.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "coll/coll_common_attributes.hpp"

namespace ccl {
class ccl_barrier_attr_impl_t : public ccl_common_op_attr_impl_t
{
public:
    using base_t = ccl_common_op_attr_impl_t;

    ccl_barrier_attr_impl_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::version>::type& version);
};
}
