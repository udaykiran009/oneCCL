#pragma once

#include "ccl_types.h"
#ifdef MULTI_GPU_SUPPORT

#ifdef __cplusplus
extern "C" {
#endif

ccl_status_t CCL_API register_allreduce_gpu_module_source(const char* source, ccl_topology_class_t topology_class);
#ifdef __cplusplus
}   /*extern C */
#endif

#endif //MULTI_GPU_SUPPORT
