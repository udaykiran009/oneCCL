#pragma once
#include "common/comm/l0/modules/a2a/allreduce_export_functions.hpp"
#include "common/comm/l0/modules/gpu_typed_module.hpp"

namespace native
{

DEFINE_SPECIFIC_GPU_MODULE_CLASS(gpu_coll_module, real_gpu_typed_module, ccl_coll_allreduce, ccl::device_topology_type::a2a_device_group, a2a_allreduce_kernel);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(gpu_coll_module, real_gpu_typed_module, ccl_coll_allreduce, ccl::device_topology_type::a2a_thread_group, a2a_allreduce_kernel);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(gpu_coll_module, real_gpu_typed_module, ccl_coll_allreduce, ccl::device_topology_type::a2a_allied_process_group, a2a_allreduce_kernel);

DEFINE_SPECIFIC_GPU_MODULE_CLASS(ipc_gpu_coll_module, ipc_gpu_typed_module, ccl_coll_allreduce, ccl::device_topology_type::a2a_device_group, a2a_allreduce_ipc);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(ipc_gpu_coll_module, ipc_gpu_typed_module, ccl_coll_allreduce, ccl::device_topology_type::a2a_thread_group, a2a_allreduce_ipc);
DEFINE_SPECIFIC_GPU_MODULE_CLASS(ipc_gpu_coll_module, ipc_gpu_typed_module, ccl_coll_allreduce, ccl::device_topology_type::a2a_allied_process_group, a2a_allreduce_ipc);

DEFINE_VIRTUAL_GPU_MODULE_CLASS(ccl_coll_allreduce, ccl::device_topology_type::a2a_device_group, a2a_allreduce_kernel);
DEFINE_VIRTUAL_GPU_MODULE_CLASS(ccl_coll_allreduce, ccl::device_topology_type::a2a_thread_group, a2a_allreduce_kernel);
DEFINE_VIRTUAL_GPU_MODULE_CLASS(ccl_coll_allreduce, ccl::device_topology_type::a2a_allied_process_group, a2a_allreduce_kernel);

}
