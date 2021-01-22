#pragma once
#include "common/comm/l0/modules/ring/allreduce_export_functions.hpp"
#include "common/comm/l0/modules/gpu_typed_module.hpp"

namespace native {

DEFINE_SPECIFIC_GPU_MODULE_CLASS(device_coll_module,
                                 real_gpu_typed_module,
                                 ccl_coll_allreduce,
                                 ccl::device_topology_type::ring,
                                 ring_allreduce_kernel,
                                 ring_allreduce_numa_kernel,
                                 ring_allreduce_scale_out_cpu_gw_kernel);

DEFINE_SPECIFIC_GPU_MODULE_CLASS(ipc_dst_device_coll_module,
                                 ipc_gpu_typed_module,
                                 ccl_coll_allreduce,
                                 ccl::device_topology_type::ring,
                                 ring_allreduce_ipc,
                                 ring_allreduce_ipc,
                                 ring_allreduce_ipc);

DEFINE_VIRTUAL_GPU_MODULE_CLASS(ccl_coll_allreduce,
                                ccl::device_topology_type::ring,
                                ring_allreduce_kernel,
                                ring_allreduce_numa_kernel,
                                ring_allreduce_scale_out_cpu_gw_kernel);
} // namespace native
