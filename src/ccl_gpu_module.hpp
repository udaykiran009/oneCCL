#pragma once

#include "oneapi/ccl/types.hpp"
#include "coll/algorithms/algorithms_enum.hpp"
#include "internal_types.hpp"

#ifdef MULTI_GPU_SUPPORT
ccl::status load_gpu_module(const std::string& path,
                            ccl::device_topology_type topo_type,
                            ccl_coll_type coll_type);
#endif //MULTI_GPU_SUPPORT