#pragma once
#include <tuple>
#include <memory>
#include <stdexcept>
#include <vector>
#include "common/comm/l0/devices/proxy_observer.hpp"

namespace native
{

template<class device>
class ccl_gpu_scaleup_proxy;

// Static interface used to register proxy_observers
template <class ctx_impl_t,
          class ...devices>
class scaleup_ctx_base
{
public:
    using own_t = scaleup_ctx_base<ctx_impl_t, devices...>;

    template<class device_t>
    using proxy_observer_ptr = proxy_observer<ccl_gpu_scaleup_proxy<device_t>>* ;//std::weak_ptr<proxy_observer<ccl_gpu_scaleup_proxy<device_t>>>;

    template<class device_t>
    using observers_container_t = std::set<proxy_observer_ptr<device_t>>;

    template<ccl::device_topology_type topology_type>
    struct observables_types : std::tuple<observers_container_t<devices>...>{};

    template<ccl::device_topology_type ...topology_type>
    using observable_topologies = std::tuple<observables_types<topology_type>...>;

    ctx_impl_t* get_this()
    {
        return static_cast<ctx_impl_t*>(this);
    }

    const ctx_impl_t* get_this() const
    {
        return static_cast<const ctx_impl_t*>(this);
    }

    template<ccl::device_topology_type topology_type, class device_t>
    void attach(proxy_observer_ptr<device_t> obj)
    {
        get_this()->attach_scaleup_proxy_observer(obj,
                                                  std::integral_constant<ccl::device_topology_type,
                                                                         topology_type> {});
    }

    template<ccl::device_topology_type topology_type, class device_t, class ...Args>
    void invoke_proxy(proxy_observer_ptr<device_t> obj, Args&&...args)
    {
        get_this()->invoke_scaleup_proxy_observer(obj,
                                                  std::integral_constant<ccl::device_topology_type,
                                                                         topology_type> {},
                                                  std::forward<Args>(args)...);
    }
};
}
