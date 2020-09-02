#pragma once
#include "common/comm/l0/modules/ring/bcast_export_functions.hpp"
#include "common/comm/l0/modules/gpu_typed_module.hpp"

namespace native {

DEFINE_SPECIFIC_GPU_MODULE_CLASS(device_coll_module,
                                 real_gpu_typed_module,
                                 ccl_coll_bcast,
                                 ccl::device_topology_type::ring,
                                 ring_bcast_kernel,
                                 ring_bcast_numa_kernel);

DEFINE_SPECIFIC_GPU_MODULE_CLASS(ipc_dst_device_coll_module,
                                 ipc_gpu_typed_module,
                                 ccl_coll_bcast,
                                 ccl::device_topology_type::ring,
                                 ring_bcast_ipc,
                                 ring_bcast_ipc);

DEFINE_VIRTUAL_GPU_MODULE_CLASS(ccl_coll_bcast,
                                ccl::device_topology_type::ring,
                                ring_bcast_kernel,
                                ring_bcast_numa_kernel);
} // namespace native
