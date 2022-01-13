#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/aliases.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "oneapi/ccl/type_traits.hpp"

#include "oneapi/ccl/coll_attr_ids.hpp"
#include "oneapi/ccl/coll_attr_ids_traits.hpp"
#include "oneapi/ccl/coll_attr.hpp"

namespace ccl {

namespace v1 {

template <class attr>
CCL_API attr ccl_empty_attr::create_empty() {
    return attr{ ccl_empty_attr::version };
}

CCL_API allgatherv_attr default_allgatherv_attr = ccl_empty_attr::create_empty<allgatherv_attr>();
CCL_API allreduce_attr default_allreduce_attr = ccl_empty_attr::create_empty<allreduce_attr>();
CCL_API alltoall_attr default_alltoall_attr = ccl_empty_attr::create_empty<alltoall_attr>();
CCL_API alltoallv_attr default_alltoallv_attr = ccl_empty_attr::create_empty<alltoallv_attr>();
CCL_API barrier_attr default_barrier_attr = ccl_empty_attr::create_empty<barrier_attr>();
CCL_API broadcast_attr default_broadcast_attr = ccl_empty_attr::create_empty<broadcast_attr>();
CCL_API reduce_attr default_reduce_attr = ccl_empty_attr::create_empty<reduce_attr>();
CCL_API reduce_scatter_attr default_reduce_scatter_attr =
    ccl_empty_attr::create_empty<reduce_scatter_attr>();

} // namespace v1

} // namespace ccl
