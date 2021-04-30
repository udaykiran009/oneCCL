#pragma once

#include "exec/thread/base_thread.hpp"
#include "sched/queue/strict_queue.hpp"
#include "sched/queue/queue.hpp"
#include "internal_types.hpp"

#include <memory>
#include <list>
#include <pthread.h>

class ccl_executor;

class ccl_worker : public ccl_base_thread {
public:
    ccl_worker() = delete;
    ccl_worker(const ccl_worker& other) = delete;
    ccl_worker& operator=(const ccl_worker& other) = delete;
    ccl_worker(size_t idx, std::unique_ptr<ccl_sched_queue> queue);
    virtual ~ccl_worker() = default;
    virtual void* get_this() override {
        return static_cast<void*>(this);
    };

    virtual const std::string& name() const override {
        static const std::string name("worker");
        return name;
    };

    void add(ccl_sched* sched);

    virtual ccl::status do_work(size_t& processed_count);

    void clear_queue();

    void reset_queue(std::unique_ptr<ccl_sched_queue>&& queue) {
        clear_queue();
        sched_queue = std::move(queue);
    }

    std::atomic<bool> should_lock;
    std::atomic<bool> is_locked;
    bool process_atl;

    void update_wait_condition(ccl_base_thread::wait_data::update_type type, size_t delta);

    bool check_wait_condition(size_t iter);
    bool check_affinity_condition(size_t iter);
    bool check_stop_condition(size_t iter);

private:
    ccl::status process_strict_sched_queue();
    ccl::status process_sched_queue(size_t& processed_count, bool process_all);
    ccl::status process_sched_bin(ccl_sched_bin* bin, size_t& processed_count);

    size_t do_work_counter = 0;

    std::unique_ptr<ccl_strict_sched_queue> strict_sched_queue;
    std::unique_ptr<ccl_sched_queue> sched_queue;
};
