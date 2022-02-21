#pragma once

#include <unordered_map>

#include "sched/entry/ze/ze_cache.hpp"
#include "sched/entry/ze/ze_primitives.hpp"
#include "sched/ze/ze_event_manager.hpp"

#include "sched/sched_timer.hpp"

namespace ccl {

namespace ze {

struct device_info {
    ze_device_handle_t device;
    uint32_t parent_idx;
    ze_device_uuid_t uuid;

    device_info(ze_device_handle_t dev, uint32_t parent_idx);
};

struct global_data_desc {
    std::vector<ze_driver_handle_t> drivers;
    std::vector<ze_context_handle_t> contexts;
    std::vector<device_info> devices;
    std::vector<ze_device_handle_t> device_handles;
    std::unique_ptr<ze::cache> cache;
    std::unordered_map<ze_context_handle_t, ccl::ze::dynamic_event_pool> dynamic_event_pools;

    std::atomic<size_t> kernel_counter{};

    global_data_desc();
    global_data_desc(const global_data_desc&) = delete;
    global_data_desc(global_data_desc&&) = delete;
    global_data_desc& operator=(const global_data_desc&) = delete;
    global_data_desc& operator=(global_data_desc&&) = delete;
    ~global_data_desc();
};

} // namespace ze
} // namespace ccl
