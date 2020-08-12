#pragma once
#include "ccl_device_communicator.hpp"

namespace ccl
{
class kvs_interface;

/* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
template<class DeviceType,
         class ContextType>
vector_class<device_communicator> create_device_communicators(
        const size_t size,
        const vector_class<DeviceType>& devices,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs)
{
}

template<class DeviceType,
         class ContextType>
vector_class<device_communicator> create_device_communicators(
        const size_t size,
        vector_class<pair_class<size_t, DeviceType>>& rank_device_map,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs)
{
}
}
