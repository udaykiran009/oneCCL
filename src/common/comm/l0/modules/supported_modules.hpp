#pragma once
#include "common/comm/l0/modules/base_entry_module.hpp"

namespace native {
// alias for topologies
template <template <ccl_coll_type, ccl::device_group_split_type, ccl::device_topology_type>
          class module_impl,
          ccl_coll_type type,
          ccl::device_group_split_type group_id,
          ccl::device_topology_type... class_ids>
using topology_device_classes_modules_for_group_id =
    std::tuple<std::shared_ptr<module_impl<type, group_id, class_ids>>...>;

template <template <ccl_coll_type, ccl::device_group_split_type, ccl::device_topology_type>
          class module_impl,
          ccl_coll_type type,
          ccl::device_group_split_type... top_types>
using topology_device_group_modules = std::tuple<
    topology_device_classes_modules_for_group_id<module_impl,
                                                 type,
                                                 top_types,
                                                 SUPPORTED_TOPOLOGY_CLASSES_DECL_LIST>...>;

// alias for coll types
template <template <ccl_coll_type, ccl::device_group_split_type, ccl::device_topology_type>
          class module_impl,
          ccl_coll_type... types>
using supported_topology_device_modules = std::tuple<
    topology_device_group_modules<module_impl, types, SUPPORTED_HW_TOPOLOGIES_DECL_LIST>...>;

// alias for implemented modules
template <template <ccl_coll_type, ccl::device_group_split_type, ccl::device_topology_type>
          class module_impl>
using supported_device_modules = supported_topology_device_modules<module_impl,
                                                                   ccl_coll_allgatherv,
                                                                   ccl_coll_allreduce,
                                                                   ccl_coll_alltoall,
                                                                   ccl_coll_alltoallv,
                                                                   ccl_coll_barrier,
                                                                   ccl_coll_bcast>;
} // namespace native
