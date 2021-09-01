#pragma once

#include <mutex>

#include "common/comm/comm_interface.hpp"

struct base_communicator : public ccl::communicator_interface {
    base_communicator(ccl::unified_device_type&& owned_device,
                      ccl::unified_context_type&& owned_ctx,
                      size_t thread_idx,
                      size_t process_idx,
                      const ccl::comm_split_attr& attr)
            : device(std::move(owned_device)),
              context(std::move(owned_ctx)),
              thread_id(thread_idx),
              process_id(process_idx),
              comm_attr(attr),
              comm_rank(),
              comm_size(),
              ready_mutex() /*,
        devices(nullptr)*/
    {}

    virtual ~base_communicator() = default;

    int rank() const override {
        return comm_rank;
    }

    int size() const override {
        return comm_size;
    }

    ccl::device_index_type get_device_path() const override {
        return device.get_id();
    }

    ccl::communicator_interface::device_t get_device() const override {
        return device.get();
    }

    ccl::communicator_interface::context_t get_context() const override {
        return context.get();
    }

    const ccl::comm_split_attr& get_comm_split_attr() const override {
        return comm_attr;
    }

    ccl::unified_device_type device;
    ccl::unified_context_type context;
    size_t thread_id;
    size_t process_id;
    const ccl::comm_split_attr comm_attr;

    //TODO add context_comm_addr to aggregate device_id,thread_id, process_id & ranks
    int comm_rank;
    int comm_size;

    mutable ccl_spinlock ready_mutex;
};
