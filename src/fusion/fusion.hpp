#pragma once

#include "common/utils/spinlock.hpp"
#include "exec/exec.hpp"
#include "sched/sched.hpp"

#include <chrono>
#include <mutex>
#include <deque>

#define CCL_FUSION_BYTES_THRESHOLD (8 * 8192)
#define CCL_FUSION_COUNT_THRESHOLD (256)
#define CCL_FUSION_BUFFER_SIZE     (CCL_FUSION_BYTES_THRESHOLD * CCL_FUSION_COUNT_THRESHOLD)
#define CCL_BUFFER_CACHE_PREALLOC  (4)

using fusion_lock_t = ccl_spinlock;

class ccl_buffer_cache
{
public:
    ccl_buffer_cache(size_t buf_size);
    ~ccl_buffer_cache();

    ccl_buffer_cache(const ccl_buffer_cache& other) = delete;
    ccl_buffer_cache& operator= (const ccl_buffer_cache& other) = delete;

    void* get();
    void release(void* buf);

    size_t get_buf_size() { return buf_size; }

private:
    size_t buf_size;
    fusion_lock_t guard{};
    std::deque<void*> free_buffers;
    std::deque<void*> all_buffers;
};

class ccl_fusion_manager
{
public:
    ccl_fusion_manager();
    ~ccl_fusion_manager();

    ccl_fusion_manager(const ccl_fusion_manager& other) = delete;
    ccl_fusion_manager& operator= (const ccl_fusion_manager& other) = delete;

    bool can_fuse(ccl_sched* sched);
    bool add(ccl_sched* sched);
    void execute();
    void release_buffer(void* buf);

private:
    ccl_sched* build_sched();
    void clear_exec_queue();
    void check_tracked_scheds();

    size_t bytes_threshold;
    size_t count_threshold;

    fusion_lock_t guard{};
    using sched_queue_t = std::deque<ccl_sched*>;
    sched_queue_t postponed_queue{};
    sched_queue_t exec_queue{};
    size_t exec_queue_sum_bytes = 0;
    ccl_buffer_cache buf_cache;
    std::list<ccl_sched*> tracked_scheds{};

    std::chrono::steady_clock::duration cycle;
    std::chrono::steady_clock::time_point last_exec_time;

    size_t stat_fused_ops = 0;
    size_t stat_fused_bytes = 0;
    size_t stat_empty_exec_calls = 0;
};
