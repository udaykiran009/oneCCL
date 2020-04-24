#pragma once
#include "common/comm/l0/modules/base_entry_module.hpp"

namespace native
{
// alias for topologies
template<template<ccl_coll_type, ccl::device_topology_type> class module_impl,
         ccl_coll_type type,
         ccl::device_topology_type... top_types>
using topology_device_modules = std::tuple<std::shared_ptr<module_impl<type, top_types>>...>;

// alias for coll types
template<template<ccl_coll_type, ccl::device_topology_type> class module_impl,
         ccl_coll_type... types>
using supported_topology_device_modules = std::tuple<topology_device_modules<module_impl,
                                                                             types,
                                                                             SUPPORTED_HW_TOPOLOGIES_DECL_LIST>...>;


// alias for implemented modules
template<template<ccl_coll_type, ccl::device_topology_type> class module_impl>
using supported_device_modules = supported_topology_device_modules<module_impl,
                                                                  ccl_coll_allgatherv,
                                                                  ccl_coll_allreduce>;
}
