#pragma once

#include "exec/thread/base_thread.hpp"
#include "sched/queue/strict_queue.hpp"
#include "sched/queue/queue.hpp"

#include <memory>
#include <list>
#include <pthread.h>

class ccl_executor;

class ccl_worker : public ccl_base_thread
{
public:
    ccl_worker() = delete;
    ccl_worker(const ccl_worker& other) = delete;
    ccl_worker& operator= (const ccl_worker& other) = delete;
    ccl_worker(ccl_executor* executor, size_t idx, std::unique_ptr<ccl_sched_queue> queue);
    virtual ~ccl_worker() = default;
    virtual void* get_this() override { return static_cast<void*>(this); };

    virtual const std::string& name() const override
    {
        static const std::string name("worker");
        return name;
    };

    void add(ccl_sched* sched);

    virtual ccl_status_t do_work(size_t& processed_count);

    std::atomic<bool> should_lock;
    std::atomic<bool> is_locked;

    void clear_queue();

    void reset_queue(std::unique_ptr<ccl_sched_queue>&& queue)
    {
        clear_queue();
        sched_queue = std::move(queue);
    }

    ccl_status_t process_strict_sched_queue();
    ccl_status_t process_sched_queue(size_t& processed_count);

private:
    ccl_executor* executor = nullptr;

    std::unique_ptr<ccl_strict_sched_queue> strict_sched_queue;
    std::unique_ptr<ccl_sched_queue> sched_queue;
};
