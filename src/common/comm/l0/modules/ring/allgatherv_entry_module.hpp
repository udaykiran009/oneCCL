#pragma once
#include "common/comm/l0/modules/ring/allgatherv_export_functions.hpp"
#include "common/comm/l0/modules/gpu_typed_module.hpp"

namespace native {

DEFINE_SPECIFIC_GPU_MODULE_CLASS(device_coll_module,
                                 real_gpu_typed_module,
                                 ccl_coll_allgatherv,
                                 ccl::device_topology_type::ring,
                                 ring_allgatherv_kernel,
                                 ring_allgatherv_numa_kernel);

DEFINE_SPECIFIC_GPU_MODULE_CLASS(ipc_dst_device_coll_module,
                                 ipc_gpu_typed_module,
                                 ccl_coll_allgatherv,
                                 ccl::device_topology_type::ring,
                                 ring_allgatherv_ipc,
                                 ring_allgatherv_ipc);

DEFINE_VIRTUAL_GPU_MODULE_CLASS(ccl_coll_allgatherv,
                                ccl::device_topology_type::ring,
                                ring_allgatherv_kernel,
                                ring_allgatherv_numa_kernel);
} // namespace native
