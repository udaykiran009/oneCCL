#pragma once

#include "common/utils/spinlock.hpp"
#include "exec/exec.hpp"
#include "sched/sched.hpp"

#include <chrono>
#include <mutex>
#include <deque>

#define ICCL_FUSION_BYTES_THRESHOLD (8 * 8192)
#define ICCL_FUSION_COUNT_THRESHOLD (256)
#define ICCL_FUSION_BUFFER_SIZE     (ICCL_FUSION_BYTES_THRESHOLD * ICCL_FUSION_COUNT_THRESHOLD)
#define ICCL_BUFFER_CACHE_PREALLOC  (4)

using fusion_lock_t = iccl_spinlock;

class iccl_buffer_cache
{
public:
    iccl_buffer_cache(size_t buf_size);
    ~iccl_buffer_cache();

    iccl_buffer_cache(const iccl_buffer_cache& other) = delete;
    iccl_buffer_cache& operator= (const iccl_buffer_cache& other) = delete;

    void* get();
    void release(void* buf);

private:
    size_t buf_size;
    fusion_lock_t guard{};
    std::deque<void*> free_buffers;
    std::deque<void*> all_buffers;
};

class iccl_fusion_manager
{
public:
    iccl_fusion_manager();
    ~iccl_fusion_manager();

    iccl_fusion_manager(const iccl_fusion_manager& other) = delete;
    iccl_fusion_manager& operator= (const iccl_fusion_manager& other) = delete;

    bool can_fuse(iccl_sched* sched);
    bool add(iccl_sched* sched);
    void execute();
    void release_buffer(void* buf);

private:
    iccl_sched* build_sched();
    void clear_exec_queue();
    void check_tracked_scheds();

    size_t bytes_threshold;
    size_t count_threshold;

    fusion_lock_t guard{};
    using sched_queue_t = std::deque<iccl_sched*>;
    sched_queue_t postponed_queue{};
    sched_queue_t exec_queue{};
    size_t exec_queue_sum_bytes = 0;
    iccl_buffer_cache buf_cache;
    std::list<iccl_sched*> tracked_scheds{};

    std::chrono::steady_clock::duration cycle;
    std::chrono::steady_clock::time_point last_exec_time;

    size_t stat_fused_ops = 0;
    size_t stat_fused_bytes = 0;
    size_t stat_empty_exec_calls = 0;
};
