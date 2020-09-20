#pragma once

#include "ccl_types.h"
#include "coll/algorithms/algorithms_enum.hpp"

#ifdef MULTI_GPU_SUPPORT

#ifdef __cplusplus
extern "C" {
#endif

ccl_status_t CCL_API register_gpu_module_source(const char* source,
                                                ccl_topology_class_t topology_class,
                                                ccl_coll_type type);

#ifdef __cplusplus
} /*extern C */
#endif

#endif //MULTI_GPU_SUPPORT
