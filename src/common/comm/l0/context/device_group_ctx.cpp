#include <sstream>

#include "common/comm/l0/devices/devices_declaration.hpp"
#include "common/comm/l0/context/device_group_ctx.hpp"
#include "common/comm/l0/context/device_storage.hpp"
#include "common/comm/l0/topology/ring/device_group_ring_creator.hpp"
#include "common/comm/l0/device_community.hpp"

#include "common/comm/l0/scheduler/device_group_scheduler.hpp"

namespace native
{

std::shared_ptr<device_group_context>
        device_group_context::create(const ccl::context_comm_addr& comm_addr,
                                     const ccl::device_indices_t& group_device_ids,
                                     device_storage& devices)
{
    std::shared_ptr<device_group_context> ret(new device_group_context(comm_addr, group_device_ids));

    //TODO More intellectual topology creation required
    //Ring
    {
        device_group_ring_topology top(*ret, devices);

        std::stringstream ss;
        auto matrix = top.build_p2p_capability_matrix(ss, group_device_ids);
        ss << "\nMatrix\n" << matrix << std::endl;

        if(!top.build(ss, comm_addr, group_device_ids, matrix))
        {
            LOG_ERROR("Cannot build DEVICE_GROUP_RING. Devices cannot communicate for current setup!\nBuild log:\n", ss.str());
            abort();
        }
        LOG_DEBUG("Device Group Context for ", comm_addr.to_string(),
                  " build RING topology. Log:\n ", ss.str());

/*        native::details::printer<device_group_ring_topology::type()> p;
        ccl_tuple_for_each(ring_device_topology->get_device_storage(), p);
        LOG_INFO("Device Group ", context_addr.to_string(), " RING topology:\n", p.to_string());
*/

    }

    //A2A
    {
        /* TODO
        auto a2a_device_topology = std::make_shared<device_community<ccl::device_topology_type::a2a_device_group>>(context_addr);
        device_group_a2a_topology top(*this, plain_gpu_comms, ring_device_topology->get_device_storage_ptr());
        std::stringstream ss;
        auto matrix = top.build_p2p_capability_matrix(ss, group_device_ids);
        ss << "\nMatrix\n" << matrix << std::endl;
        if(!top.build(ss, 0, group_device_ids, matrix))
        {
            LOG_ERROR("Cannot build DEVICE_GROUP_RING. Devices cannot communicate for current setup!\nBuild log:\n", ss.str());
            abort();
        }
        LOG_DEBUG("Device Group Context for ", context_addr.to_string(), " build RING topology. Log:\n ", ss.str());
        native::details::printer<device_group_ring_topology::type()> p;
        ccl_tuple_for_each(ring_device_topology->get_device_storage(), p);
        LOG_INFO("Device Group ", context_addr.to_string(), " RING topology:\n", p.to_string());
        LOG_INFO("Device Group ", context_addr.to_string(), " A2A topology:\nTODO!");
        //remember
        std::get<ccl::device_topology_class::a2a_class>(device_topology) = a2a_device_topology;
        */
    }

    return ret;
}

device_group_context::device_group_context(const ccl::context_comm_addr& comm_addr,
                                           const ccl::device_indices_t& group_device_ids):
  device_indices(group_device_ids),
  context_addr(comm_addr)
{
    //scheduler
    scheduler_impl.reset(new device_group_scheduler);
}

device_group_context::~device_group_context()
{
}

const ccl::device_indices_t& device_group_context::get_group_device_indices() const
{
    return device_indices;
}



void device_group_context::attach_scaleup_proxy_observer(proxy_observer<ccl_gpu_scaleup_proxy<ccl_gpu_comm>>* observer,
                                       std::integral_constant<ccl::device_topology_type,
                                                              ccl::device_topology_type::device_group_ring> val)
{
    register_observer_impl<ccl::device_topology_type::device_group_ring>(observer);
}
void device_group_context::attach_scaleup_proxy_observer(proxy_observer<ccl_gpu_scaleup_proxy<ccl_virtual_gpu_comm>>* observer,
                                       std::integral_constant<ccl::device_topology_type,
                                                              ccl::device_topology_type::device_group_ring> val)
{
    register_observer_impl<ccl::device_topology_type::device_group_ring>(observer);
}


void device_group_context::attach_scaleup_proxy_observer(proxy_observer<ccl_gpu_scaleup_proxy<ccl_gpu_comm>>* observer,
                                       std::integral_constant<ccl::device_topology_type,
                                                              ccl::device_topology_type::device_group_torn_apart_ring> val)
{
    register_observer_impl<ccl::device_topology_type::device_group_torn_apart_ring>(observer);
}
void device_group_context::attach_scaleup_proxy_observer(proxy_observer<ccl_gpu_scaleup_proxy<ccl_virtual_gpu_comm>>* observer,
                                       std::integral_constant<ccl::device_topology_type,
                                                              ccl::device_topology_type::device_group_torn_apart_ring> val)
{
    register_observer_impl<ccl::device_topology_type::device_group_torn_apart_ring>(observer);
}


void device_group_context::invoke_scaleup_proxy_observer(proxy_observer<ccl_gpu_scaleup_proxy<ccl_gpu_comm>>* observer,
                                       std::integral_constant<ccl::device_topology_type,
                                                              ccl::device_topology_type::device_group_ring> val)
{
    auto &topologu_specific_observers =
            std::get<top_to_index(ccl::device_topology_type::device_group_ring)>(observables);
    observers_container_t<ccl_gpu_comm>& container = std::get<ccl_gpu_comm::type_idx()>(topologu_specific_observers);
    auto it = container.find(observer);
    if(it == container.end())
    {
        throw std::runtime_error(std::string("invalid proxy: ") + observer->get_this()->get_device().to_string());
    }

    throw std::runtime_error(std::string("Valid proxy: ") + observer->get_this()->get_device().to_string());
}

void device_group_context::invoke_scaleup_proxy_observer(proxy_observer<ccl_gpu_scaleup_proxy<ccl_virtual_gpu_comm>>* observer,
                                       std::integral_constant<ccl::device_topology_type,
                                                              ccl::device_topology_type::device_group_ring> val)
{
    throw std::runtime_error(std::string("Valid proxy: ") + observer->get_this()->get_device().to_string());
}

void device_group_context::invoke_scaleup_proxy_observer(proxy_observer<ccl_gpu_scaleup_proxy<ccl_gpu_comm>>* observer,
                                       std::integral_constant<ccl::device_topology_type,
                                                              ccl::device_topology_type::device_group_torn_apart_ring> val)
{
    throw std::runtime_error(std::string("Valid proxy: ") + observer->get_this()->get_device().to_string());
}

void device_group_context::invoke_scaleup_proxy_observer(proxy_observer<ccl_gpu_scaleup_proxy<ccl_virtual_gpu_comm>>* observer,
                                       std::integral_constant<ccl::device_topology_type,
                                                              ccl::device_topology_type::device_group_torn_apart_ring> val)
{
    throw std::runtime_error(std::string("Valid proxy: ") + observer->get_this()->get_device().to_string());
}
}
