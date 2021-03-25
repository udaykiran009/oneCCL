#pragma once
#include "common/comm/l0/modules/a2a/allreduce_export_functions.hpp"
#include "common/comm/l0/modules/gpu_typed_module.hpp"

namespace native {
DEFINE_SPECIFIC_GPU_MODULE_CLASS(device_coll_module,
                                 real_gpu_typed_module,
                                 ccl_coll_allreduce,
                                 ccl::device_topology_type::a2a,
                                 a2a::allreduce::main_kernel,
                                 a2a::allreduce::numa_kernel,
                                 a2a::allreduce::scale_out_cpu_gw_kernel);

DEFINE_SPECIFIC_GPU_MODULE_CLASS(ipc_dst_device_coll_module,
                                 ipc_gpu_typed_module,
                                 ccl_coll_allreduce,
                                 ccl::device_topology_type::a2a,
                                 a2a::allreduce::ipc_kernel,
                                 a2a::allreduce::ipc_kernel,
                                 a2a::allreduce::ipc_kernel);

DEFINE_VIRTUAL_GPU_MODULE_CLASS(ccl_coll_allreduce,
                                ccl::device_topology_type::a2a,
                                a2a::allreduce::main_kernel,
                                a2a::allreduce::numa_kernel,
                                a2a::allreduce::scale_out_cpu_gw_kernel);

} // namespace native
