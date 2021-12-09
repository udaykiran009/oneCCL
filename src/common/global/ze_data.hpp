#include "sched/entry/ze/ze_cache.hpp"
#include "sched/entry/ze/ze_primitives.hpp"

#include "sched/sched_timer.hpp"

namespace ccl {

namespace ze {

struct global_data_desc {
    std::vector<ze_driver_handle_t> driver_list;
    std::vector<ze_context_handle_t> context_list;
    std::vector<ze_device_handle_t> device_list;
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
