#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <pthread.h>

#include "common/log/log.hpp"
#include "internal_types.hpp"

#define CCL_UNDEFINED_CPU_ID (-1)

class ccl_base_thread {
public:
    ccl_base_thread(size_t idx, void* (*progress_function)(void*))
            : should_stop(false),
              started(false),
              wait(0),
              idx(idx),
              start_affinity(CCL_UNDEFINED_CPU_ID),
              progress_function(progress_function) {}

    ccl_base_thread() = delete;
    ~ccl_base_thread() = default;

    ccl_base_thread(const ccl_base_thread&) = delete;
    ccl_base_thread(ccl_base_thread&&) = delete;

    ccl_base_thread& operator=(const ccl_base_thread&) = delete;
    ccl_base_thread& operator=(ccl_base_thread&&) = delete;

    ccl::status start(int affinity);
    ccl::status stop();

    size_t get_idx() {
        return idx;
    }
    virtual void* get_this() {
        return static_cast<void*>(this);
    };

    int get_start_affinity() {
        return start_affinity;
    }
    int get_affinity();

    virtual const std::string& name() const {
        static const std::string name("base_thread");
        return name;
    };

    std::atomic<bool> should_stop;
    std::atomic<bool> started;

    struct wait_data {
        std::mutex mtx;
        std::condition_variable var;

        /*
           wakeup condition
           for worker threads value == active_op_count
           so wakeup will happen if value is > 0
        */
        size_t value;

        enum update_type { increment, decrement };

        wait_data(size_t value) : value(value) {}
    };

    wait_data wait;

private:
    ccl::status set_affinity(int affinity);

    const size_t idx;

    int start_affinity;
    void* (*progress_function)(void*);
    pthread_t thread{};
};
