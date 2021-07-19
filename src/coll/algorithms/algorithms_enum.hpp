#pragma once
#include "common/utils/enums.hpp"

#include "oneapi/ccl/types.hpp"

#define CCL_COLL_LIST \
    ccl_coll_allgatherv, ccl_coll_allreduce, ccl_coll_alltoall, ccl_coll_alltoallv, \
        ccl_coll_barrier, ccl_coll_bcast, ccl_coll_reduce, ccl_coll_reduce_scatter, \
        ccl_coll_sparse_allreduce

enum ccl_coll_allgatherv_algo {
    ccl_coll_allgatherv_direct,
    ccl_coll_allgatherv_naive,
    ccl_coll_allgatherv_ring,
    ccl_coll_allgatherv_flat,
    ccl_coll_allgatherv_multi_bcast,

    ccl_coll_allgatherv_last_value
};

enum ccl_coll_allreduce_algo {
    ccl_coll_allreduce_direct,
    ccl_coll_allreduce_rabenseifner,
    ccl_coll_allreduce_starlike,
    ccl_coll_allreduce_ring,
    ccl_coll_allreduce_ring_rma,
    ccl_coll_allreduce_double_tree,
    ccl_coll_allreduce_recursive_doubling,
    ccl_coll_allreduce_2d,
    ccl_coll_allreduce_topo_ring,

    ccl_coll_allreduce_last_value
};

enum ccl_coll_alltoall_algo {
    ccl_coll_alltoall_direct,
    ccl_coll_alltoall_naive,
    ccl_coll_alltoall_scatter,
    ccl_coll_alltoall_scatter_barrier,

    ccl_coll_alltoall_last_value
};

enum ccl_coll_alltoallv_algo {
    ccl_coll_alltoallv_direct,
    ccl_coll_alltoallv_naive,
    ccl_coll_alltoallv_scatter,
    ccl_coll_alltoallv_scatter_barrier,

    ccl_coll_alltoallv_last_value
};

enum ccl_coll_barrier_algo {
    ccl_coll_barrier_direct,
    ccl_coll_barrier_ring,

    ccl_coll_barrier_last_value
};

enum ccl_coll_bcast_algo {
    ccl_coll_bcast_direct,
    ccl_coll_bcast_ring,
    ccl_coll_bcast_double_tree,
    ccl_coll_bcast_naive,

    ccl_coll_bcast_last_value
};

enum ccl_coll_reduce_algo {
    ccl_coll_reduce_direct,
    ccl_coll_reduce_rabenseifner,
    ccl_coll_reduce_tree,
    ccl_coll_reduce_double_tree,

    ccl_coll_reduce_last_value
};

enum ccl_coll_reduce_scatter_algo {
    ccl_coll_reduce_scatter_direct,
    ccl_coll_reduce_scatter_ring,

    ccl_coll_reduce_scatter_last_value
};

enum ccl_coll_sparse_allreduce_algo {
    ccl_coll_sparse_allreduce_ring,
    ccl_coll_sparse_allreduce_mask,
    ccl_coll_sparse_allreduce_3_allgatherv,

    ccl_coll_sparse_allreduce_last_value
};

enum ccl_coll_type {
    ccl_coll_allgatherv,
    ccl_coll_allreduce,
    ccl_coll_alltoall,
    ccl_coll_alltoallv,
    ccl_coll_barrier,
    ccl_coll_bcast,
    ccl_coll_reduce,
    ccl_coll_reduce_scatter,
    ccl_coll_sparse_allreduce,
    ccl_coll_last_regular = ccl_coll_sparse_allreduce,

    ccl_coll_internal,
    ccl_coll_partial,

    ccl_coll_last_value
};

// Currently ccl_coll_type is used in both compile-time and run-time contexts, so
// need to have both versions of the check.
// It's possible to have a constexpr function, but it requires some features from c++14
// (e.g. multiple returns in constexpr functions)

template <ccl_coll_type ctype, class Enable = void>
struct is_reduction_coll_type : std::false_type {};

// Reduction types
template <ccl_coll_type ctype>
struct is_reduction_coll_type<
    ctype,
    typename std::enable_if<ctype == ccl_coll_allreduce || ctype == ccl_coll_reduce ||
                            ctype == ccl_coll_reduce_scatter>::type> : std::true_type {};

bool ccl_coll_type_is_reduction(ccl_coll_type ctype);
const char* ccl_coll_type_to_str(ccl_coll_type type);

#define CCL_COLL_TYPE_LIST \
    ccl_coll_type::ccl_coll_allgatherv, ccl_coll_type::ccl_coll_allreduce, \
        ccl_coll_type::ccl_coll_alltoall, ccl_coll_type::ccl_coll_alltoallv, \
        ccl_coll_type::ccl_coll_barrier, ccl_coll_type::ccl_coll_bcast, \
        ccl_coll_type::ccl_coll_reduce, ccl_coll_type::ccl_coll_reduce_scatter, \
        ccl_coll_type::ccl_coll_sparse_allreduce

enum ccl_coll_reduction {
    sum,
    prod,
    min,
    max,
    //custom, TODO: make support of custom reduction in *.cl

    last_value
};

#define REDUCE_TYPES \
    ccl::reduction::sum, ccl::reduction::prod, ccl::reduction::min, \
        ccl::reduction::max /*, ccl::reduction::custom*/

using ccl_reductions =
    utils::enum_to_str<static_cast<typename std::underlying_type<ccl::reduction>::type>(
        ccl::reduction::custom)>;
inline const std::string reduction_to_str(ccl::reduction reduction_type) {
    return ccl_reductions({ "sum", "prod", "min", "max" }).choose(reduction_type, "INVALID_VALUE");
}
