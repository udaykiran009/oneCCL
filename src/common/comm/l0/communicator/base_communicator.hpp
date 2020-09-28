#pragma once
#include <mutex>

#include "common/comm/comm_interface.hpp"
//TODO #include "sched/gpu_sched.hpp"
#include "common/comm/l0/comm_context_id.hpp"

struct base_communicator : public ccl::communicator_interface {
    //TODO using group_comm_storage = native::specific_indexed_device_storage;

    base_communicator(ccl::unified_device_type&& owned_device,
                      size_t thread_idx,
                      size_t process_idx,
                      const ccl::comm_split_attr& attr)
            : device(std::move(owned_device)),
              thread_id(thread_idx),
              process_id(process_idx),
              comm_attr(attr),
              comm_rank(),
              comm_size(),
              ready_mutex() /*,
        devices(nullptr)*/
    {}

    virtual ~base_communicator() = default;

    size_t rank() const override {
        return comm_rank;
    }

    size_t size() const override {
        return comm_size;
    }

    ccl::device_index_type get_device_path() const override {
        return device.get_id();
    }

    ccl::communicator_interface::device_t get_device() override {
        return device.get();
    }

    ccl::communicator_interface::context_t get_context() override {
        return context.get();
    }

    const ccl::comm_split_attr& get_comm_split_attr() const override {
        return comm_attr;
    }

    const ccl::group_unique_key& get_comm_group_id() const override {
        return owner_id;
    }

    void set_comm_group_id(ccl::group_unique_key id) {
        owner_id = id;
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
    ccl::unified_device_context_type context;
    size_t thread_id;
    size_t process_id;
    const ccl::comm_split_attr comm_attr;

    //TODO add context_comm_addr to aggregate device_id,thread_id, process_id & ranks
    size_t comm_rank;
    size_t comm_size;

    mutable ccl_spinlock ready_mutex;

    ccl::group_unique_key owner_id;
};
