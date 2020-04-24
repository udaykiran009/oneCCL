#pragma once
#include "ccl.hpp"
#include "ccl_type_traits.hpp"
#include "common/comm/l0/communicator/typed_base_communicator.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"
#include "common/comm/l0/context/thread_group_ctx.hpp"
#include "common/comm/l0/context/process_group_ctx.hpp"

template<ccl::device_topology_type topology>
typed_base_communicator<topology>::typed_base_communicator(ccl::unified_device_type&& owned_device,
                                                           size_t thread_idx, size_t process_idx,
                                                           const ccl::shared_comm_device_attr_t& attr) :
 base_communicator(std::move(owned_device),
                   thread_idx, process_idx/*, comm_attr*/, attr)
{
    LOG_INFO("sheduled for create, device id: ", device.get_id(), ", thread_id: ",thread_idx, ", process id:", process_idx);
}

template<ccl::device_topology_type topology>
void typed_base_communicator<topology>::initialize_comm_addr(const ccl::device_index_type& device_id,
                                                             std::shared_ptr<native::device_community<topology>> new_community)
{
    native::details::rank_getter<topology> initializer(device_id,
                                                   new_community->registered_device_id);

    {
        std::unique_lock<ccl_spinlock> lock(ready_mutex);
        device_community_impl = new_community;

        if (!device_community_impl->get_device_storage_ptr())
        {
            std::string err_str;
            {
                std::stringstream str;
                ccl_logger::format(str, "Cannot initialize comm_addr for device id: ", device_id,
                                       " on topology: ", ::to_string(self_t::get_topology_type()),
                                       ", empty device storage has got from context");
                err_str = str.str();
            }
            LOG_ERROR(err_str);
            throw std::runtime_error(err_str);
        }
        ccl_tuple_for_each(device_community_impl->get_device_storage(), initializer);
    }

    comm_rank = initializer.rank;
    comm_size = initializer.size;

    //TODO
    //ADD communicator device binding

    LOG_INFO("Communicator finalized. Rank (", comm_rank, "/", comm_size,
             ") on {dev: ", device_id, ", thr: ",thread_id, ", proc: ", process_id, "}");
}

template<ccl::device_topology_type topology>
bool typed_base_communicator<topology>::is_ready() const
{
    if(!device_community_impl.get())
    {
        std::unique_lock<ccl_spinlock> lock(ready_mutex);
        return device_community_impl.get();
    }
    return true;
}

template<ccl::device_topology_type topology>
ccl::device_topology_type typed_base_communicator<topology>::get_topology_type() const
{
    return self_t::topology_type();
}

template<ccl::device_topology_type topology>
template<class device_t>
size_t typed_base_communicator<topology>::get_device_count() const
{
    return ccl_tuple_get<native::indexed_device_container<device_t>>(device_community_impl->get_device_storage()).size();
}

template<ccl::device_topology_type topology>
template<class device_t>
native::indexed_device_container<device_t>& typed_base_communicator<topology>::get_devices()
{
    return std::get<device_t::type_idx()>(device_community_impl->get_device_storage());
}

template<ccl::device_topology_type topology>
std::string typed_base_communicator<topology>::to_string() const
{
    native::details::printer<self_t::topology_type()> p;
    ccl_tuple_for_each(device_community_impl->get_device_storage(), p);
    return std::string("Rank (") + std::to_string(rank()) + "/" + std::to_string(size()) +
            "\nTopology: " + ::to_string(self_t::get_topology_type()) + ":\n" + p.to_string();
}
