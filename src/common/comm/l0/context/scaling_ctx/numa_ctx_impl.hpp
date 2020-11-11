#pragma once
#include "common/comm/l0/context/scaling_ctx/numa_ctx.hpp"
#include "common/utils/tuple.hpp"

namespace native {

#define TEMPLATE_DECL_ARG class Impl, ccl::device_topology_type... types
#define TEMPLATE_DEF_ARG  Impl, types...

template <TEMPLATE_DECL_ARG>
template <ccl::device_topology_type class_id, class device_t>
void numa_ctx<TEMPLATE_DEF_ARG>::register_observer_impl(size_t rank_addr, observer_t<device_t>* observer_ptr) {
    observer::container_t<observer_t<device_t>>& container =
        scaling_ctx_base_t::template get_types_container<observer_t<device_t>, class_id>(
            observables);
    container.insert(observer_ptr);


    throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - not implemented");
}

template <TEMPLATE_DECL_ARG>
void numa_ctx<TEMPLATE_DEF_ARG>::invoke_ctx_observer(
    observer_t<ccl_gpu_comm>* observer_ptr,
    std::integral_constant<ccl::device_topology_type, ccl::device_topology_type::ring> val) {
    observer::container_t<observer_t<ccl_gpu_comm>>& container =
        scaling_ctx_base_t::template get_types_container<observer_t<ccl_gpu_comm>,
                                                         ccl::device_topology_type::ring>(
            observables);

    auto it = container.find(observer_ptr);
    if (it == container.end()) {
        throw std::runtime_error(std::string("invalid proxy: ") + observer_ptr->to_string());
    }

    throw std::runtime_error(std::string("Valid proxy: ") + observer_ptr->to_string());
}

template <TEMPLATE_DECL_ARG>
void numa_ctx<TEMPLATE_DEF_ARG>::invoke_ctx_observer(
    observer_t<ccl_virtual_gpu_comm>* observer_ptr,
    std::integral_constant<ccl::device_topology_type, ccl::device_topology_type::ring> val) {
    throw std::runtime_error(std::string("Valid proxy: ") + observer_ptr->to_string());
}

template <TEMPLATE_DECL_ARG>
void numa_ctx<TEMPLATE_DEF_ARG>::invoke_ctx_observer(
    observer_t<ccl_gpu_comm>* observer_ptr,
    std::integral_constant<ccl::device_topology_type, ccl::device_topology_type::a2a> val) {
    throw std::runtime_error(std::string("Valid proxy: ") + observer_ptr->to_string());
}

template <TEMPLATE_DECL_ARG>
void numa_ctx<TEMPLATE_DEF_ARG>::invoke_ctx_observer(
    observer_t<ccl_virtual_gpu_comm>* observer_ptr,
    std::integral_constant<ccl::device_topology_type, ccl::device_topology_type::a2a> val) {
    throw std::runtime_error(std::string("Valid proxy: ") + observer_ptr->to_string());
}

#undef TEMPLATE_DECL_ARG
#undef TEMPLATE_DEF_ARG

} // namespace native
