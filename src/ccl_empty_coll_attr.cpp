#include "ccl_types.hpp"
#include "ccl_aliases.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_type_traits.hpp"

#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "ccl_coll_attr.hpp"

// Core file with PIMPL implementation
//#include "coll_attr_impl.hpp"
//#include "coll/coll_attributes.hpp"


namespace ccl
{
template<class attr>
attr ccl_empty_attr::create_empty()
{
    return attr{ccl_empty_attr::version};
}

allgatherv_attr_t default_allgather_attr = ccl_empty_attr::create_empty<allgatherv_attr_t>();
allreduce_attr_t default_allreduce_attr = ccl_empty_attr::create_empty<allreduce_attr_t>();
alltoall_attr_t default_alltoall_attr = ccl_empty_attr::create_empty<alltoall_attr_t>();
alltoallv_attr_t default_alltoallv_attr = ccl_empty_attr::create_empty<alltoallv_attr_t>();
bcast_attr_t default_bcast_attr = ccl_empty_attr::create_empty<bcast_attr_t>();
reduce_attr_t default_reduce_attr = ccl_empty_attr::create_empty<reduce_attr_t>();
reduce_scatter_attr_t default_reduce_scatter_attr = ccl_empty_attr::create_empty<reduce_scatter_attr_t>();
sparse_allreduce_attr_t default_sparse_allreduce_attr_t = ccl_empty_attr::create_empty<sparse_allreduce_attr_t>();
barrier_attr_t default_barrier_attr_t = ccl_empty_attr::create_empty<barrier_attr_t>();

}
