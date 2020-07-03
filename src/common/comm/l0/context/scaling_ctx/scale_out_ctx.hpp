#pragma once
#include "common/comm/l0/context/base_scaling_ctx.hpp"

namespace native {

class ccl_gpu_comm;
class ccl_virtual_gpu_comm;

template <class device>
class ccl_scaleout_proxy;

template <class device>
class ccl_gpu_scaleup_proxy;

template <class device>
class ccl_numa_proxy;

template <class Impl, ccl::device_topology_type... types>
class scale_out_ctx
        : public observer::base_scaling_ctx<
              scale_out_ctx<Impl, types...>,
              ccl_scaleout_proxy<ccl_gpu_comm>,
              ccl_scaleout_proxy<ccl_virtual_gpu_comm>,
              ccl_scaleout_proxy<ccl_numa_proxy<ccl_gpu_comm>>,
              ccl_scaleout_proxy<ccl_numa_proxy<ccl_virtual_gpu_comm>>,
              ccl_scaleout_proxy<ccl_gpu_scaleup_proxy<ccl_gpu_comm>>,
              ccl_scaleout_proxy<ccl_gpu_scaleup_proxy<ccl_virtual_gpu_comm>>,
              ccl_scaleout_proxy<ccl_gpu_scaleup_proxy<ccl_numa_proxy<ccl_gpu_comm>>>,
              ccl_scaleout_proxy<ccl_gpu_scaleup_proxy<ccl_numa_proxy<ccl_virtual_gpu_comm>>>> {
public:
    using context_impl = Impl;

    template <class device_t>
    using observer_t = ccl_scaleout_proxy<device_t>;

    using scaling_ctx_base_t = observer::base_scaling_ctx<
        scale_out_ctx<Impl, types...>,
        observer_t<ccl_gpu_comm>,
        observer_t<ccl_virtual_gpu_comm>,
        observer_t<ccl_numa_proxy<ccl_gpu_comm>>,
        observer_t<ccl_numa_proxy<ccl_virtual_gpu_comm>>,
        observer_t<ccl_gpu_scaleup_proxy<ccl_gpu_comm>>,
        observer_t<ccl_gpu_scaleup_proxy<ccl_virtual_gpu_comm>>,
        observer_t<ccl_gpu_scaleup_proxy<ccl_numa_proxy<ccl_gpu_comm>>>,
        observer_t<ccl_gpu_scaleup_proxy<ccl_numa_proxy<ccl_virtual_gpu_comm>>>>;

    using observable_scale_up_topologies =
        typename scaling_ctx_base_t::template observable_topologies<types...>;

    observable_scale_up_topologies observables;

    //observer subject interface implementations
    template <class device_t, ccl::device_topology_type topology_type>
    void attach_ctx_observer(observer_t<device_t>* observer_ptr,
                             std::integral_constant<ccl::device_topology_type, topology_type> val) {
        register_observer_impl<topology_type>(observer_ptr);
    }

    void invoke_ctx_observer(
        observer_t<ccl_gpu_comm>* observer_ptr,
        std::integral_constant<ccl::device_topology_type, ccl::device_topology_type::ring> val);
    void invoke_ctx_observer(
        observer_t<ccl_virtual_gpu_comm>* observer_ptr,
        std::integral_constant<ccl::device_topology_type, ccl::device_topology_type::ring> val);

    void invoke_ctx_observer(
        observer_t<ccl_gpu_comm>* observer_ptr,
        std::integral_constant<ccl::device_topology_type, ccl::device_topology_type::a2a> val);
    void invoke_ctx_observer(
        observer_t<ccl_virtual_gpu_comm>* observer_ptr,
        std::integral_constant<ccl::device_topology_type, ccl::device_topology_type::a2a> val);

private:
    template <ccl::device_topology_type topology_type, class device_t>
    void register_observer_impl(observer_t<device_t>* observer_ptr); /*
    {
        auto &topologu_specific_observers = std::get<topology_index>(observables);
        container_t<device_t>& container = std::get<device_t::type_idx()>(topologu_specific_observers);
        container.insert(observer);
    }*/
};
} // namespace native
