#pragma once
#include "common/comm/l0/device_types.hpp"

namespace native
{

class ccl_gpu_comm;
class ccl_ipc_gpu_comm;
class ccl_virtual_gpu_comm;
template<class device_t>
class ccl_thread_comm;
template<class device_t>
class ccl_ipc_source_gpu_comm;
template<class device_t>
class ccl_numa_proxy;
template<class device_t>
class ccl_gpu_scaleup_proxy;
template<class device_t>
class ccl_scaleout_proxy;

#define SUPPORTED_DEVICES_DECL_LIST                                                                 \
            ccl_gpu_comm, ccl_virtual_gpu_comm,                                                     \
            ccl_thread_comm<ccl_gpu_comm>,                                                          \
            ccl_thread_comm<ccl_virtual_gpu_comm>,                                                  \
            ccl_ipc_source_gpu_comm<ccl_gpu_comm>,                                                  \
            ccl_ipc_source_gpu_comm<ccl_virtual_gpu_comm>,                                          \
            ccl_ipc_gpu_comm,                                                                       \
            ccl_numa_proxy<ccl_gpu_comm>,                                                           \
            ccl_numa_proxy<ccl_virtual_gpu_comm>,                                                   \
            ccl_gpu_scaleup_proxy<ccl_gpu_comm>,                                                    \
            ccl_gpu_scaleup_proxy<ccl_virtual_gpu_comm>,                                            \
            ccl_gpu_scaleup_proxy<ccl_numa_proxy<ccl_gpu_comm>>,                                    \
            ccl_gpu_scaleup_proxy<ccl_numa_proxy<ccl_virtual_gpu_comm>>,                            \
            ccl_scaleout_proxy<ccl_gpu_comm>,                                                       \
            ccl_scaleout_proxy<ccl_virtual_gpu_comm>,                                               \
            ccl_scaleout_proxy<ccl_numa_proxy<ccl_gpu_comm>>,                                       \
            ccl_scaleout_proxy<ccl_numa_proxy<ccl_virtual_gpu_comm>>,                               \
            ccl_scaleout_proxy<ccl_gpu_scaleup_proxy<ccl_gpu_comm>>,                                \
            ccl_scaleout_proxy<ccl_gpu_scaleup_proxy<ccl_virtual_gpu_comm>>,                        \
            ccl_scaleout_proxy<ccl_gpu_scaleup_proxy<ccl_numa_proxy<ccl_gpu_comm>>>,                \
            ccl_scaleout_proxy<ccl_gpu_scaleup_proxy<ccl_numa_proxy<ccl_virtual_gpu_comm>>>

}
