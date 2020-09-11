#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/comm/single_device_communicator/single_device_base.hpp"

#define TEMPLATE_DECL_ARG class comm_impl, class communicator_traits
#define TEMPLATE_DEF_ARG  comm_impl, communicator_traits

template <TEMPLATE_DECL_ARG>
typed_single_device_base_communicator<TEMPLATE_DEF_ARG>::typed_single_device_base_communicator(
    ccl::unified_device_type&& owned_device,
    size_t thread_idx,
    size_t process_idx,
    const ccl::device_comm_split_attr& attr)
        : base_communicator(std::move(owned_device),
                            thread_idx,
                            process_idx /*, comm_attr*/,
                            attr) {
    try {
        LOG_INFO("sheduled for create, device id: ",
                 device.get_id(),
                 ", thread_id: ",
                 thread_idx,
                 ", process id:",
                 process_idx);
    }
    catch (...) {
        LOG_INFO("sheduled for create single device communicator , thread_id: ",
                 thread_idx,
                 ", process id:",
                 process_idx);
    }
}

template <TEMPLATE_DECL_ARG>
bool typed_single_device_base_communicator<TEMPLATE_DEF_ARG>::is_ready() const {
    return true;
}

template <TEMPLATE_DECL_ARG>
ccl::device_group_split_type
typed_single_device_base_communicator<TEMPLATE_DEF_ARG>::get_topology_type() const {
    return self_t::topology_type();
}

template <TEMPLATE_DECL_ARG>
ccl::device_topology_type
typed_single_device_base_communicator<TEMPLATE_DEF_ARG>::get_topology_class() const {
    return self_t::topology_class();
}

template <TEMPLATE_DECL_ARG>
std::string typed_single_device_base_communicator<TEMPLATE_DEF_ARG>::to_string() const {
    return std::string("single device communicator, rank (") + std::to_string(rank()) + "/" +
           std::to_string(size());
}

#undef TEMPLATE_DECL_ARG
#undef TEMPLATE_DEF_ARG
