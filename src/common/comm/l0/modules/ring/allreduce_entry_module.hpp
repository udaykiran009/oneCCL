#pragma once
#include "common/comm/l0/modules/ring/allreduce_export_functions.hpp"
#include "common/comm/l0/modules/gpu_typed_module.hpp"

namespace native {

DEFINE_SPECIFIC_GPU_MODULE_CLASS(device_coll_module,
                                 real_gpu_typed_module,
                                 ccl_coll_allreduce,
                                 ccl::device_topology_type::ring,
                                 ring::allreduce::main_kernel,
                                 ring::allreduce::numa_kernel,
                                 ring::allreduce::scale_out_cpu_gw_kernel);

DEFINE_SPECIFIC_GPU_MODULE_CLASS(ipc_dst_device_coll_module,
                                 ipc_gpu_typed_module,
                                 ccl_coll_allreduce,
                                 ccl::device_topology_type::ring,
                                 ring::allreduce::ipc_kernel,
                                 ring::allreduce::ipc_kernel,
                                 ring::allreduce::ipc_kernel);

DEFINE_VIRTUAL_GPU_MODULE_CLASS(ccl_coll_allreduce,
                                ccl::device_topology_type::ring,
                                ring::allreduce::main_kernel,
                                ring::allreduce::numa_kernel,
                                ring::allreduce::scale_out_cpu_gw_kernel);
} // namespace native
