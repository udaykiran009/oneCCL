#pragma once

#include "common/comm/l0/device_community_holder.hpp"
#include "common/comm/l0/device_community_holder_utils.hpp"

namespace native {
#define TEMPLATE_DECL_ARG ccl::group_split_type group_id, ccl::device_topology_type... class_id
#define TEMPLATE_DEF_ARG  group_id, class_id...

// community impl
template <ccl::device_topology_type class_id>
template <ccl::group_split_type group_id, class... DeviceTypes>
void device_community_container<class_id>::bind_device_by_id(
    const ccl::device_index_type& device_id,
    ccl::context_comm_addr& registered_addr,
    device_variant_t<DeviceTypes...>& out_device_binder,
    size_t preferred_rank) {
    storage->template bind_device_by_id<group_id>(
        device_id, registered_addr, out_device_binder, preferred_rank);
}

template <ccl::group_split_type group_id, class... DeviceTypes>
void device_community_container<ccl::device_topology_type::ring>::bind_device_by_id(
    const ccl::device_index_type& device_id,
    ccl::context_comm_addr& registered_addr,
    device_variant_t<DeviceTypes...>& out_device_binder,
    size_t preferred_rank) {
    for (auto it = closed_rings.begin(); it != closed_rings.end(); ++it) {
        (*it)->template bind_device_by_id<group_id>(
            device_id, registered_addr, out_device_binder, preferred_rank);
    }

    for (auto it = torn_apart_rings.begin(); it != torn_apart_rings.end(); ++it) {
        (*it)->template bind_device_by_id<group_id>(
            device_id, registered_addr, out_device_binder, preferred_rank);
    }
}

// implementation
template <TEMPLATE_DECL_ARG>
template <ccl::device_topology_type requested_id>
const device_community_container<requested_id>&
device_group_community_holder<TEMPLATE_DEF_ARG>::get_community() const {
    return std::get<requested_id>(typed_communities);
}

template <TEMPLATE_DECL_ARG>
template <ccl::device_topology_type requested_id>
device_community_container<requested_id>&
device_group_community_holder<TEMPLATE_DEF_ARG>::get_community() {
    return const_cast<device_community_container<requested_id>&>(
        static_cast<const self_t*>(this)->get_community<requested_id>());
}

template <TEMPLATE_DECL_ARG>
std::string device_group_community_holder<TEMPLATE_DEF_ARG>::to_string() const {
    std::stringstream ss;
    detail::device_community_container_print_helper<group_id> p(ss);
    ccl_tuple_for_each(typed_communities, p);
    return ss.str();
}

template <TEMPLATE_DECL_ARG>
template <ccl::device_topology_type requested_class_id, class... DeviceTypes>
void device_group_community_holder<TEMPLATE_DEF_ARG>::bind_device_by_id(
    const ccl::device_index_type& device_id,
    ccl::context_comm_addr& registered_addr,
    device_variant_t<DeviceTypes...>& out_binder,
    size_t preferred_rank) {
    device_community_container<requested_class_id>& container =
        this->template get_community<requested_class_id>();
    container->bind_device_by_id(device_id, registered_addr, out_binder, preferred_rank);
}

#undef TEMPLATE_DECL_ARG
#undef TEMPLATE_DEF_ARG
} // namespace native
