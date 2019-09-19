#pragma once

#define CCL_COLL_LIST \
  ccl_coll_allgatherv, ccl_coll_allreduce, \
  ccl_coll_barrier, ccl_coll_bcast, ccl_coll_reduce, \
  ccl_coll_sparse_allreduce

enum ccl_coll_allgatherv_algo
{
    ccl_coll_allgatherv_direct,
    ccl_coll_allgatherv_naive,
    ccl_coll_allgatherv_flat,
    ccl_coll_allgatherv_multi_bcast,

    ccl_coll_allgatherv_last_value
};

enum ccl_coll_allreduce_algo
{
    ccl_coll_allreduce_direct,
    ccl_coll_allreduce_rabenseifner,
    ccl_coll_allreduce_starlike,
    ccl_coll_allreduce_ring,
    ccl_coll_allreduce_ring_rma,
    ccl_coll_allreduce_double_tree,
    ccl_coll_allreduce_recursive_doubling,

    ccl_coll_allreduce_last_value
};

enum ccl_coll_barrier_algo
{
    ccl_coll_barrier_direct,
    ccl_coll_barrier_ring,

    ccl_coll_barrier_last_value
};

enum ccl_coll_bcast_algo
{
    ccl_coll_bcast_direct,
    ccl_coll_bcast_ring,
    ccl_coll_bcast_double_tree,
    ccl_coll_bcast_naive,

    ccl_coll_bcast_last_value
};

enum ccl_coll_reduce_algo
{
    ccl_coll_reduce_direct,
    ccl_coll_reduce_rabenseifner,
    ccl_coll_reduce_tree,
    ccl_coll_reduce_double_tree,

    ccl_coll_reduce_last_value
};

enum ccl_coll_sparse_allreduce_algo
{
    ccl_coll_sparse_allreduce_basic,
    ccl_coll_sparse_allreduce_size,
    ccl_coll_sparse_allreduce_mask,
    ccl_coll_sparse_allreduce_3_allgatherv,

    ccl_coll_sparse_allreduce_last_value
};

enum ccl_coll_type
{
    ccl_coll_allgatherv,
    ccl_coll_allreduce,
    ccl_coll_barrier,
    ccl_coll_bcast,
    ccl_coll_reduce,
    ccl_coll_sparse_allreduce,
    ccl_coll_internal,

    ccl_coll_last_value
};