#pragma once
#include "oneapi/ccl/ccl_device_communicator.hpp"
#include "common/comm/l0/comm_context.hpp"
#include "common/comm/l0/comm_context_storage.hpp"


namespace ccl
{
class kvs_interface;

/* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
template<class DeviceType,
         class ContextType>
vector_class<device_communicator> device_communicator::create_device_communicators(
        const size_t cluster_devices_size,
        const vector_class<DeviceType>& local_devices,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs)
{
    vector_class<device_communicator> ret;
    throw std::runtime_error(std::string(__FUNCTION__) + " - not implemented");
    return ret;
}

using rank_t = size_t;

template<class DeviceType,
         class ContextType>
vector_class<device_communicator> device_communicator::create_device_communicators(
        const size_t cluster_devices_size, /*global devics count*/
        const vector_class<pair_class<rank_t, DeviceType>>& local_rank_device_map,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs)
{
    vector_class<rank_t> local_thread_ranks;
    local_thread_ranks.reserve(local_rank_device_map.size());
    std::transform(local_rank_device_map.begin(), local_rank_device_map.end(), std::back_inserter(local_thread_ranks),
                    [](const typename vector_class<pair_class<rank_t, DeviceType>>::value_type& val)
                    {
                        return val.first;
                    });
    group_context::comm_group_t thread_group = group_context::instance().group_by_kvs(local_thread_ranks, cluster_devices_size, kvs);


    vector_class<DeviceType> local_thread_devices;
    local_thread_devices.reserve(local_rank_device_map.size());
    std::transform(local_rank_device_map.begin(), local_rank_device_map.end(), std::back_inserter(local_thread_devices),
                    [](const typename vector_class<pair_class<rank_t, DeviceType>>::value_type& val)
                    {
                        return val.second;
                    });

    auto ret = thread_group->create_communicators(local_thread_devices);
    return ret;
}


template<class DeviceType,
         class ContextType>
vector_class<device_communicator> device_communicator::create_device_communicators(
        const size_t cluster_devices_size, /*global devics count*/
        const map_class<rank_t, DeviceType>& local_rank_device_map,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs)

{
    vector_class<rank_t> local_thread_ranks;
    local_thread_ranks.reserve(local_rank_device_map.size());
    std::transform(local_rank_device_map.begin(), local_rank_device_map.end(), std::back_inserter(local_thread_ranks),
                    [](const typename map_class<rank_t, DeviceType>::value_type& val)
                    {
                        return val.first;
                    });
    group_context::comm_group_t thread_group = group_context::instance().group_by_kvs(local_thread_ranks, cluster_devices_size, kvs);


    vector_class<DeviceType> local_thread_devices;
    local_thread_devices.reserve(local_rank_device_map.size());
    std::transform(local_rank_device_map.begin(), local_rank_device_map.end(), std::back_inserter(local_thread_devices),
                    [](const typename  map_class<rank_t, DeviceType>::value_type& val)
                    {
                        return val.second;
                    });

    auto ret = thread_group->create_communicators(local_thread_devices);
    return ret;
}

}
