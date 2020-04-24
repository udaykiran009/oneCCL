#pragma once
#include "common/comm/l0/modules/ring/allreduce_export_functions.hpp"
#include "common/comm/l0/modules/gpu_typed_module.hpp"

namespace native
{

DEFINE_SPECIFIC_GPU_MODULE_CLASS(gpu_coll_module, real_gpu_typed_module, ccl_coll_allreduce, ccl::device_topology_type::device_group_ring, ring_allreduce_kernel);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(gpu_coll_module, real_gpu_typed_module, ccl_coll_allreduce, ccl::device_topology_type::thread_group_ring, ring_allreduce_kernel);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(gpu_coll_module, real_gpu_typed_module, ccl_coll_allreduce, ccl::device_topology_type::allied_process_group_ring, ring_allreduce_kernel);

DEFINE_SPECIFIC_GPU_MODULE_CLASS(ipc_gpu_coll_module, ipc_gpu_typed_module, ccl_coll_allreduce, ccl::device_topology_type::device_group_ring, ring_allreduce_ipc);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(ipc_gpu_coll_module, ipc_gpu_typed_module, ccl_coll_allreduce, ccl::device_topology_type::thread_group_ring, ring_allreduce_ipc);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(ipc_gpu_coll_module, ipc_gpu_typed_module, ccl_coll_allreduce, ccl::device_topology_type::allied_process_group_ring, ring_allreduce_ipc);

DEFINE_VIRTUAL_GPU_MODULE_CLASS(ccl_coll_allreduce, ccl::device_topology_type::device_group_ring, ring_allreduce_kernel);
DEFINE_VIRTUAL_GPU_MODULE_CLASS(ccl_coll_allreduce, ccl::device_topology_type::thread_group_ring, ring_allreduce_kernel);
DEFINE_VIRTUAL_GPU_MODULE_CLASS(ccl_coll_allreduce, ccl::device_topology_type::allied_process_group_ring, ring_allreduce_kernel);

}
