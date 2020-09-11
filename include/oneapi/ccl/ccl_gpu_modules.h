#pragma once

#include "oneapi/ccl/ccl_types.hpp"
#ifdef MULTI_GPU_SUPPORT

#ifdef __cplusplus
extern "C" {
#endif

void CCL_API register_allreduce_gpu_module_source(const char* source,
                                                  ccl::device_topology_type type);
#ifdef __cplusplus
} /*extern C */
#endif

#endif //MULTI_GPU_SUPPORT
