#pragma once

#include "oneapi/ccl/ccl_types.hpp"
<<<<<<< HEAD
#include "coll/algorithms/algorithms_enum.hpp"

=======
>>>>>>> ffc3a2b9e14b387e58918449284645c170346325
#ifdef MULTI_GPU_SUPPORT

#ifdef __cplusplus
extern "C" {
#endif

<<<<<<< HEAD
ccl_status_t CCL_API register_gpu_module_source(const char* source,
                                                ccl::device_topology_type topology_class,
                                                ccl_coll_type type);

=======
void CCL_API register_allreduce_gpu_module_source(const char* source,
                                                  ccl::device_topology_type type);
>>>>>>> ffc3a2b9e14b387e58918449284645c170346325
#ifdef __cplusplus
} /*extern C */
#endif

#endif //MULTI_GPU_SUPPORT
