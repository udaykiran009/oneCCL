#pragma once

#include "oneapi/ccl/ccl_types.hpp"
#include "coll/algorithms/algorithms_enum.hpp"

#ifdef MULTI_GPU_SUPPORT
ccl::status CCL_API register_gpu_module_source(const char* source,
                                                ccl::device_topology_type topology_class,
                                                ccl_coll_type type);
#endif //MULTI_GPU_SUPPORT