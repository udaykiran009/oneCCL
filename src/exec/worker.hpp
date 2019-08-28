#pragma once

#include "sched/sched_queue.hpp"
#include "ccl_thread.hpp"
#include <memory>
#include <list>
#include <pthread.h>

class ccl_executor;

class ccl_worker : public ccl_thread
{
public:
    ccl_worker() = delete;
    ccl_worker(const ccl_worker& other) = delete;
    ccl_worker& operator= (const ccl_worker& other) = delete;
    ccl_worker(ccl_executor* executor, size_t idx, std::unique_ptr<ccl_sched_queue> queue);
    virtual ~ccl_worker() = default;
    virtual void* get_this() override { return static_cast<void*>(this);};
    virtual std::string name() override { return "worker";};

    void add(ccl_sched* sched);

    virtual ccl_status_t do_work(size_t& processed_count);

    std::atomic<bool> should_lock;
    std::atomic<bool> is_locked;
    void clear_data_queue();
    void reset_data_queue(std::unique_ptr<ccl_sched_queue>&& queue)
    {
        data_queue = std::move(queue);
    }
private:
    ccl_executor* executor = nullptr;
    std::unique_ptr<ccl_sched_queue> data_queue;
};
