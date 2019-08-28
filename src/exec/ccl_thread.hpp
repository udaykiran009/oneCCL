#pragma once

#include "ccl.h"
#include "common/log/log.hpp"

class ccl_thread
{
public:
    ccl_thread() = delete;
    ccl_thread(const ccl_thread& other) = delete;
    ccl_thread& operator= (const ccl_thread& other) = delete;
    ccl_thread(size_t idx, void*(*progress_function)(void*)) :
               idx(idx), progress_function(progress_function)
    { }
    ccl_status_t start();
    ccl_status_t stop();
    ccl_status_t pin(int proc_id);
    size_t get_idx() { return idx; }
    virtual void* get_this() { return static_cast<void*>(this);};
    virtual std::string name() { return "thread";};

    const size_t idx;
    pthread_t thread{};

    void*(*progress_function)(void*);
};