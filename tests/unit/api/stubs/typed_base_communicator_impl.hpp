#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/comm/l0/communicator/typed_base_communicator.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"

#define TEMPLATE_DECL_ARG \
    class comm_impl, ccl::group_split_type topology, ccl::device_topology_type class_id, \
        class communicator_traits
#define TEMPLATE_DEF_ARG comm_impl, topology, class_id, communicator_traits

template <TEMPLATE_DECL_ARG>
typed_base_communicator<TEMPLATE_DEF_ARG>::typed_base_communicator(
    ccl::unified_device_type&& owned_device,
    ccl::unified_context_type&& ctx,
    size_t thread_idx,
    size_t process_idx,
    const ccl::comm_split_attr& attr)
        : base_communicator(std::move(owned_device), std::move(ctx),
                            thread_idx,
                            process_idx /*, comm_attr*/,
                            attr) {}

template <TEMPLATE_DECL_ARG>
void typed_base_communicator<TEMPLATE_DEF_ARG>::initialize_comm_addr(
    const ccl::device_index_type& device_id,
    native::device_community_container<class_id>& new_community) {}

template <TEMPLATE_DECL_ARG>
bool typed_base_communicator<TEMPLATE_DEF_ARG>::is_ready() const {
    return true;
}

template <TEMPLATE_DECL_ARG>
ccl::group_split_type typed_base_communicator<TEMPLATE_DEF_ARG>::get_topology_type() const {
    return self_t::topology_type();
}

template <TEMPLATE_DECL_ARG>
ccl::device_topology_type typed_base_communicator<TEMPLATE_DEF_ARG>::get_topology_class() const {
    return self_t::topology_class();
}

template <TEMPLATE_DECL_ARG>
std::string typed_base_communicator<TEMPLATE_DEF_ARG>::to_string() const {
    return {};
}

template <TEMPLATE_DECL_ARG>
std::shared_ptr<ccl::communicator_interface>
typed_base_communicator<TEMPLATE_DEF_ARG>::split(const ccl::comm_split_attr& attr) {
    return {};
}

#undef TEMPLATE_DECL_ARG
#undef TEMPLATE_DEF_ARG
