#pragma once
#include "common/comm/l0/modules/ring/reduce_scatter_export_functions.hpp"
#include "common/comm/l0/modules/gpu_typed_module.hpp"

namespace native {

DEFINE_SPECIFIC_GPU_MODULE_CLASS(device_coll_module,
                                 real_gpu_typed_module,
                                 ccl_coll_reduce_scatter,
                                 ccl::device_topology_type::ring,
                                 ring::reduce_scatter::main_kernel,
                                 ring::reduce_scatter::numa_kernel,
                                 ring::reduce_scatter::scale_out_cpu_gw_kernel);

DEFINE_SPECIFIC_GPU_MODULE_CLASS(ipc_dst_device_coll_module,
                                 ipc_gpu_typed_module,
                                 ccl_coll_reduce_scatter,
                                 ccl::device_topology_type::ring,
                                 ring::reduce_scatter::ipc_kernel,
                                 ring::reduce_scatter::ipc_kernel,
                                 ring::reduce_scatter::ipc_kernel);

DEFINE_VIRTUAL_GPU_MODULE_CLASS(ccl_coll_reduce_scatter,
                                ccl::device_topology_type::ring,
                                ring::reduce_scatter::main_kernel,
                                ring::reduce_scatter::numa_kernel,
                                ring::reduce_scatter::scale_out_cpu_gw_kernel);
} // namespace native
