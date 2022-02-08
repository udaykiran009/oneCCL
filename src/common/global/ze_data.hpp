#pragma once

#include "sched/entry/ze/ze_cache.hpp"
#include "sched/entry/ze/ze_primitives.hpp"

#include "sched/sched_timer.hpp"

namespace ccl {

namespace ze {

struct device_info {
    ze_device_handle_t device;
    uint32_t parent_idx;

    device_info(ze_device_handle_t dev, uint32_t parent_idx)
            : device(dev),
              parent_idx(parent_idx){};
};

struct global_data_desc {
    std::vector<ze_driver_handle_t> driver_list;
    std::vector<ze_context_handle_t> context_list;
    std::vector<device_info> device_list;
    std::vector<ze_device_handle_t> device_handles;
    std::unique_ptr<ze::cache> cache;

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
