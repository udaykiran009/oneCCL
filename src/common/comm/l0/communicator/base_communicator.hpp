#pragma once
#include <mutex>

#include "common/comm/comm_interface.hpp"
#include "sched/gpu_sched.hpp"


struct base_communicator : public ccl::communicator_interface
{
    using group_comm_storage = native::specific_indexed_device_storage;

    base_communicator(ccl::unified_device_type&& owned_device,
                      size_t thread_idx, size_t process_idx,
                      const ccl::device_comm_split_attr_t& attr) :
        device(std::move(owned_device)),
        thread_id(thread_idx),
        process_id(process_idx),
        comm_attr(attr),
        comm_rank(),
        comm_size(),
        ready_mutex()/*,
        devices(nullptr)*/
    {
    }

    virtual ~base_communicator() = default;

    size_t rank() const override
    {
        return comm_rank;
    }

    size_t size() const override
    {
        return comm_size;
    }

    ccl::device_index_type get_device_path() const override
    {
        return device.get_id();
    }

    ccl::communicator_interface::device_t get_device() override
    {
        return device.get();
    }

    ccl::communicator_interface::context_t get_context() override
    {
        //TODO not implemented
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - not implemented");
        return context_t{};
    }

    ccl::comm_attr_t get_comm_split_attr() const override
    {
        return std::static_pointer_cast<ccl::ccl_comm_split_attr>(comm_attr);
    }

    const ccl::device_comm_split_attr_t& get_comm_split_attr() const override
    {
        return comm_attr;
    }
/*
    virtual bool is_ready() const
    {
        if(!devices)
        {
            std::unique_lock<ccl_spinlock> lock(ready_mutex);
            return devices;
        }
        return true;
    }
*/
    ccl::unified_device_type device;
    size_t thread_id;
    size_t process_id;
    const ccl::device_comm_split_attr_t comm_attr;

    //TODO add context_comm_addr to aggregate device_id,thread_id, process_id & ranks
    size_t comm_rank;
    size_t comm_size;

    mutable ccl_spinlock ready_mutex;
  //  group_comm_storage* devices;
};
