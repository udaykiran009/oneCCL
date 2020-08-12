#include "ccl_device_type_traits.hpp"
#include "ccl_comm_split_attr_ids.hpp"

namespace ccl
{

//TODO I do not want to rename 90% f code at now
using device_group_split_type = ccl_device_group_split_type;

#define SUPPORTED_HW_TOPOLOGIES_DECL_LIST \
    ccl::device_group_split_type::thread, ccl::device_group_split_type::process, \
        ccl::device_group_split_type::cluster

#define SUPPORTED_TOPOLOGY_CLASSES_DECL_LIST \
    ccl::device_topology_type::ring, ccl::device_topology_type::a2a
}
