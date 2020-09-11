#pragma once
#include <mutex>
#include <condition_variable>

namespace native {
struct signal_context {
    std::mutex thread_group_mutex;
    std::condition_variable thread_group_sync_condition;
    bool communicator_ready = false;
};
} // namespace native
