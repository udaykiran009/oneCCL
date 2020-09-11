#pragma once
#include "common/comm/l0/modules/a2a/allreduce_export_functions.hpp"
#include "common/comm/l0/modules/gpu_typed_module.hpp"

namespace native {
DEFINE_SPECIFIC_GPU_MODULE_CLASS(device_coll_module,
                                 real_gpu_typed_module,
                                 ccl_coll_allreduce,
                                 ccl::device_topology_type::a2a,
                                 a2a_allreduce_kernel,
                                 a2a_allreduce_numa_kernel);

DEFINE_SPECIFIC_GPU_MODULE_CLASS(ipc_dst_device_coll_module,
                                 ipc_gpu_typed_module,
                                 ccl_coll_allreduce,
                                 ccl::device_topology_type::a2a,
                                 a2a_allreduce_ipc,
                                 a2a_allreduce_ipc);

DEFINE_VIRTUAL_GPU_MODULE_CLASS(ccl_coll_allreduce,
                                ccl::device_topology_type::a2a,
                                a2a_allreduce_kernel,
                                a2a_allreduce_numa_kernel);

/*
DEFINE_SPECIFIC_GPU_MODULE_CLASS(a2a_gpu_coll_module, real_gpu_typed_module,
                                 a2a_allreduce_kernel, a2a_allreduce_numa_kernel);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(a2a_gpu_coll_module, real_gpu_typed_module,
                                 a2a_allreduce_kernel, a2a_allreduce_numa_kernel);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(a2a_gpu_coll_module, real_gpu_typed_module,
                                 a2a_allreduce_kernel, a2a_allreduce_numa_kernel);

DEFINE_SPECIFIC_GPU_MODULE_CLASS(a2a_ipc_gpu_coll_module, ipc_gpu_typed_module,
                                 a2a_allreduce_ipc, a2a_allreduce_ipc);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(a2a_ipc_gpu_coll_module, ipc_gpu_typed_module,
                                 a2a_allreduce_ipc, a2a_allreduce_ipc);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(a2a_ipc_gpu_coll_module, ipc_gpu_typed_module,
                                 a2a_allreduce_ipc, a2a_allreduce_ipc);

DEFINE_SPECIFIC_GPU_MODULE_CLASS(a2a_virtual_gpu_coll_module, virtual_gpu_typed_module,
                                a2a_allreduce_kernel, a2a_allreduce_numa_kernel);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(a2a_virtual_gpu_coll_module, virtual_gpu_typed_module,
                                a2a_allreduce_kernel, a2a_allreduce_numa_kernel);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(a2a_virtual_gpu_coll_module, virtual_gpu_typed_module,
                                a2a_allreduce_kernel, a2a_allreduce_numa_kernel);
*/
} // namespace native
