#include "coll/selection/selection.hpp"
#include "common/comm/host_communicator/host_communicator.hpp"
#include "common/global/global.hpp"

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
#include <CL/sycl/backend/level_zero.hpp>
#include "sched/entry/gpu/ze_primitives.hpp"
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

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

static bool ccl_is_device_side_algo(ccl_coll_algo algo, const ccl_selector_param& param) {
    CCL_THROW_IF_NOT(algo.has_value(), "empty algo value");

    if (param.ctype == ccl_coll_allgatherv) {
        return algo.allgatherv == ccl_coll_allgatherv_topo_a2a;
    }
    else if (param.ctype == ccl_coll_allreduce) {
        return algo.allreduce == ccl_coll_allreduce_topo_ring ||
               algo.allreduce == ccl_coll_allreduce_topo_a2a;
    }
    else if (param.ctype == ccl_coll_reduce) {
        return algo.reduce == ccl_coll_reduce_topo_ring;
    }
    else if (param.ctype == ccl_coll_bcast) {
        return algo.bcast == ccl_coll_bcast_topo_ring;
    }

    return false;
}

bool ccl_is_device_side_algo(const ccl_selector_param& param) {
#ifndef CCL_ENABLE_SYCL
    return false;
#endif // CCL_ENABLE_SYCL

    if ((param.ctype != ccl_coll_allgatherv) && (param.ctype != ccl_coll_allreduce) &&
        (param.ctype != ccl_coll_bcast) && (param.ctype != ccl_coll_reduce)) {
        return false;
    }

    ccl_coll_algo algo{};
    auto& selector = ccl::global_data::get().algorithm_selector;

    if (param.ctype == ccl_coll_allgatherv) {
        algo.allgatherv = selector->get<ccl_coll_allgatherv>(param);
    }
    else if (param.ctype == ccl_coll_allreduce) {
        algo.allreduce = selector->get<ccl_coll_allreduce>(param);
    }
    else if (param.ctype == ccl_coll_bcast) {
        algo.bcast = selector->get<ccl_coll_bcast>(param);
    }
    else if (param.ctype == ccl_coll_reduce) {
        algo.reduce = selector->get<ccl_coll_reduce>(param);
    }

    return ccl_is_device_side_algo(algo, param);
}

static bool is_family1_card(const ccl_selector_param& param) {
    bool result = false;
#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    if (param.stream && param.stream->get_backend() == sycl::backend::level_zero) {
        auto sycl_device = param.stream->get_native_stream().get_device();
        auto device = sycl_device.template get_native<sycl::backend::level_zero>();
        if (ccl::ze::get_device_family(device) == ccl::ze::device_family::family1) {
            result = true;
        }
    }
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE
    return result;
}

bool ccl_can_use_topo_a2a_algo(const ccl_selector_param& param) {
    if ((param.ctype != ccl_coll_allreduce) && (param.ctype != ccl_coll_allgatherv)) {
        return false;
    }

    bool is_sycl_buf = false;
    bool is_device_buf = true;
    bool is_l0_backend = false;

    size_t local_proc_count = ccl::global_data::get().executor->get_local_proc_count();
    int comm_size = param.comm->size();

#ifdef CCL_ENABLE_SYCL
    is_sycl_buf = param.is_sycl_buf;
    if (param.buf && param.stream) {
        auto ctx = param.stream->get_native_stream().get_context();
        is_device_buf =
            (sycl::get_pointer_type(param.buf, ctx) == sycl::usm::alloc::device) ? true : false;
    }
#ifdef CCL_ENABLE_ZE
    if (param.stream && param.stream->get_backend() == sycl::backend::level_zero) {
        is_l0_backend = true;
    }
#endif // CCL_ENABLE_ZE
#endif // CCL_ENABLE_SYCL

    bool is_single_node = static_cast<size_t>(comm_size) == local_proc_count;

    if ((comm_size < 2) || !is_single_node ||
        (!param.stream || param.stream->get_type() != stream_type::gpu) || is_sycl_buf ||
        !is_device_buf || !is_l0_backend || is_family1_card(param) ||
        ccl::global_data::env().enable_fusion || ccl::global_data::env().enable_unordered_coll ||
        (ccl::global_data::env().priority_mode != ccl_priority_none) ||
        (ccl::global_data::env().worker_count != 1)) {
        return false;
    }

    return true;
}

bool ccl_can_use_topo_ring_algo(const ccl_selector_param& param) {
    if ((param.ctype != ccl_coll_allreduce) && (param.ctype != ccl_coll_bcast) &&
        (param.ctype != ccl_coll_reduce)) {
        return false;
    }

    bool is_sycl_buf = false;
    bool is_device_buf = true;
    bool is_l0_backend = false;

    size_t local_proc_count = ccl::global_data::get().executor->get_local_proc_count();
    int comm_size = param.comm->size();

#ifdef CCL_ENABLE_SYCL
    is_sycl_buf = param.is_sycl_buf;
    if (param.buf && param.stream) {
        auto ctx = param.stream->get_native_stream().get_context();
        is_device_buf =
            (sycl::get_pointer_type(param.buf, ctx) == sycl::usm::alloc::device) ? true : false;
    }
#ifdef CCL_ENABLE_ZE
    if (param.stream && param.stream->get_backend() == sycl::backend::level_zero) {
        is_l0_backend = true;
    }
#endif // CCL_ENABLE_ZE
#endif // CCL_ENABLE_SYCL

    bool is_single_node = static_cast<size_t>(comm_size) == local_proc_count;
    bool is_onesided = (comm_size == 2) && is_single_node;

    if ((((param.ctype == ccl_coll_bcast) || (param.ctype == ccl_coll_reduce)) &&
         ((comm_size < 2) || (local_proc_count == 1))) ||
        ((param.ctype == ccl_coll_allreduce) && (comm_size <= 2) && (local_proc_count == 1)) ||

        // because of ze_ring_allreduce_entry and ze_a2a_allgatherv_entry
        (!is_onesided && (param.ctype == ccl_coll_allreduce) && is_family1_card(param)) ||
        (!is_onesided && !is_single_node && (local_proc_count % 2 != 0)) ||

        // need subcomms support from atl/mpi
        (!is_single_node && (ccl::global_data::env().atl_transport == ccl_atl_mpi)) ||

        !param.stream || (param.stream->get_type() != stream_type::gpu) || is_sycl_buf ||
        !is_device_buf || !is_l0_backend || ccl::global_data::env().enable_fusion ||
        ccl::global_data::env().enable_unordered_coll ||
        (ccl::global_data::env().priority_mode != ccl_priority_none) ||
        (ccl::global_data::env().worker_count != 1)) {
        return false;
    }

    return true;
}

bool ccl_can_use_datatype(ccl_coll_algo algo, const ccl_selector_param& param) {
    // regular datatype, don't need to check for an additional support
    if (param.dtype.idx() != ccl::datatype::bfloat16 &&
        param.dtype.idx() != ccl::datatype::float16) {
        return true;
    }

    bool can_use = true;

    bool device_side_algo = ccl_is_device_side_algo(algo, param);

    // algorithms running on device side support fp16 and bf16 both
    // so we don't need to require their support on the host
    if (!device_side_algo) {
        if (param.dtype.idx() == ccl::datatype::bfloat16) {
            bool bf16_hw_support =
                ccl::global_data::env().bf16_impl_type != ccl_bf16_no_hardware_support;
            bool bf16_compiler_support =
                ccl::global_data::env().bf16_impl_type != ccl_bf16_no_compiler_support;

            can_use = bf16_compiler_support && bf16_hw_support;

            if (!can_use) {
                LOG_DEBUG("BF16 datatype is requested for ",
                          ccl_coll_type_to_str(param.ctype),
                          " running on CPU but not fully supported: hw: ",
                          bf16_hw_support,
                          " compiler: ",
                          bf16_compiler_support);
            }
        }
        else if (param.dtype.idx() == ccl::datatype::float16) {
            bool fp16_hw_support =
                ccl::global_data::env().fp16_impl_type != ccl_fp16_no_hardware_support;
            bool fp16_compiler_support =
                ccl::global_data::env().fp16_impl_type != ccl_fp16_no_compiler_support;

            can_use = fp16_hw_support && fp16_compiler_support;

            if (!can_use) {
                LOG_DEBUG("FP16 datatype is requested for ",
                          ccl_coll_type_to_str(param.ctype),
                          " running on CPU but not fully supported: hw: ",
                          fp16_hw_support,
                          " compiler: ",
                          fp16_compiler_support);
            }
        }
    }

    return can_use;
}
