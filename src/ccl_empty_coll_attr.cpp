#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"

#include "oneapi/ccl/ccl_coll_attr_ids.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_coll_attr.hpp"

namespace ccl {

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
CCL_API sparse_allreduce_attr default_sparse_allreduce_attr =
    ccl_empty_attr::create_empty<sparse_allreduce_attr>();

} // namespace ccl
