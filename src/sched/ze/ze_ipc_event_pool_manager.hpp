#pragma once

#include "common/stream/stream.hpp"
#include "sched/entry/ze/ze_primitives.hpp"

#include <ze_api.h>

namespace ccl {

namespace ze {

class ipc_event_pool_manager {
public:
    ipc_event_pool_manager() = default;
    ipc_event_pool_manager(const ipc_event_pool_manager&) = delete;
    ipc_event_pool_manager& operator=(const ipc_event_pool_manager&) = delete;
    ~ipc_event_pool_manager() {
        clear();
    }

    void init(const ccl_stream* init_stream);
    void clear();

    ze_event_pool_handle_t create(size_t event_count);

private:
    ze_context_handle_t context{};
    std::vector<std::pair<ze_event_pool_handle_t, ze_event_pool_desc_t>> event_pool_info{};
};

} // namespace ze
} // namespace ccl
