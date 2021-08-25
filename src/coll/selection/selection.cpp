#include "coll/selection/selection.hpp"
#include "common/global/global.hpp"

bool ccl_is_direct_algo(const ccl_selector_param& param) {
    bool res = false;

    auto& selector = ccl::global_data::get().algorithm_selector;

    if (param.ctype == ccl_coll_allgatherv) {
        res = (selector->get<ccl_coll_allgatherv>(param) == ccl_coll_allgatherv_direct);
    }
    else if (param.ctype == ccl_coll_allreduce) {
        res = (selector->get<ccl_coll_allreduce>(param) == ccl_coll_allreduce_direct);
    }
    else if (param.ctype == ccl_coll_alltoall) {
        res = (selector->get<ccl_coll_alltoall>(param) == ccl_coll_alltoall_direct);
    }
    else if (param.ctype == ccl_coll_alltoallv) {
        res = (selector->get<ccl_coll_alltoallv>(param) == ccl_coll_alltoallv_direct);
    }
    else if (param.ctype == ccl_coll_barrier) {
        res = (selector->get<ccl_coll_barrier>(param) == ccl_coll_barrier_direct);
    }
    else if (param.ctype == ccl_coll_bcast) {
        res = (selector->get<ccl_coll_bcast>(param) == ccl_coll_bcast_direct);
    }
    else if (param.ctype == ccl_coll_reduce) {
        res = (selector->get<ccl_coll_reduce>(param) == ccl_coll_reduce_direct);
    }
    else if (param.ctype == ccl_coll_reduce_scatter) {
        res = (selector->get<ccl_coll_reduce_scatter>(param) == ccl_coll_reduce_scatter_direct);
    }

    return res;
}

bool ccl_is_topo_ring_algo(const ccl_selector_param& param) {
#ifndef CCL_ENABLE_SYCL
    return false;
#endif // CCL_ENABLE_SYCL

    if ((param.ctype != ccl_coll_allreduce) && (param.ctype != ccl_coll_bcast) &&
        (param.ctype != ccl_coll_reduce)) {
        return false;
    }

    bool res = false;

    auto& selector = ccl::global_data::get().algorithm_selector;

    if (param.ctype == ccl_coll_allreduce) {
        res = (selector->get<ccl_coll_allreduce>(param) == ccl_coll_allreduce_topo_ring);
    }
    else if (param.ctype == ccl_coll_bcast) {
        res = (selector->get<ccl_coll_bcast>(param) == ccl_coll_bcast_topo_ring);
    }
    else if (param.ctype == ccl_coll_reduce) {
        res = (selector->get<ccl_coll_reduce>(param) == ccl_coll_reduce_topo_ring);
    }

    return res;
}

bool ccl_can_use_topo_ring_algo(const ccl_selector_param& param) {
    if ((param.ctype != ccl_coll_allreduce) && (param.ctype != ccl_coll_bcast) &&
        (param.ctype != ccl_coll_reduce)) {
        return false;
    }

    bool is_sycl_buf = false;
    bool is_l0_backend = false;

#ifdef CCL_ENABLE_SYCL
    is_sycl_buf = param.is_sycl_buf;
#ifdef MULTI_GPU_SUPPORT
    if (param.stream && param.stream->get_backend() == sycl::backend::level_zero)
        is_l0_backend = true;
#endif // MULTI_GPU_SUPPORT
#endif // CCL_ENABLE_SYCL

    if ((param.comm->size() != 2 && param.comm->size() != 4) ||
        (param.comm->size() != ccl::global_data::get().executor->get_local_proc_count()) ||
        (!param.stream || param.stream->get_type() != stream_type::gpu) || is_sycl_buf ||
        !is_l0_backend || ccl::global_data::env().enable_fusion ||
        ccl::global_data::env().enable_unordered_coll ||
        (ccl::global_data::env().priority_mode != ccl_priority_none) ||
        (ccl::global_data::env().worker_count != 1) ||
        (ccl::global_data::env().atl_transport == ccl_atl_mpi &&
         param.ctype == ccl_coll_allreduce)) {
        return false;
    }

    return true;
}
