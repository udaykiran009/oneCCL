#pragma once

#include "common/utils/spinlock.hpp"
#include "exec/exec.hpp"
#include "sched/sched.hpp"

#include <chrono>
#include <mutex>
#include <deque>

#define MLSL_FUSION_BYTES_THRESHOLD (8 * 8192)
#define MLSL_FUSION_COUNT_THRESHOLD (256)
#define MLSL_FUSION_BUFFER_SIZE     (MLSL_FUSION_BYTES_THRESHOLD * MLSL_FUSION_COUNT_THRESHOLD)
#define MLSL_BUFFER_CACHE_PREALLOC  (4)

using fusion_lock_t = mlsl_spinlock;

class mlsl_buffer_cache
{
public:
    mlsl_buffer_cache(size_t buf_size);
    ~mlsl_buffer_cache();

    mlsl_buffer_cache(const mlsl_buffer_cache& other) = delete;
    mlsl_buffer_cache& operator= (const mlsl_buffer_cache& other) = delete;

    void* get();
    void release(void* buf);

private:
    size_t buf_size;
    fusion_lock_t guard{};
    std::deque<void*> free_buffers;
    std::deque<void*> all_buffers;
};

class mlsl_fusion_manager
{
public:
    mlsl_fusion_manager();
    ~mlsl_fusion_manager();

    mlsl_fusion_manager(const mlsl_fusion_manager& other) = delete;
    mlsl_fusion_manager& operator= (const mlsl_fusion_manager& other) = delete;

    bool can_fuse(mlsl_sched* sched);
    bool add(mlsl_sched* sched);
    void execute();
    void release_buffer(void* buf);

private:
    mlsl_sched* build_sched();
    void clear_exec_queue();
    void check_tracked_scheds();

    size_t bytes_threshold;
    size_t count_threshold;

    fusion_lock_t guard{};
    using sched_queue_t = std::deque<mlsl_sched*>;
    sched_queue_t postponed_queue{};
    sched_queue_t exec_queue{};
    size_t exec_queue_sum_bytes = 0;
    mlsl_buffer_cache buf_cache;
    std::list<mlsl_sched*> tracked_scheds{};

    std::chrono::steady_clock::duration cycle;
    std::chrono::steady_clock::time_point last_exec_time;

    size_t stat_fused_ops = 0;
    size_t stat_fused_bytes = 0;
    size_t stat_empty_exec_calls = 0;
};
