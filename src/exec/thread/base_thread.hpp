#pragma once

#include "ccl.h"
#include "common/log/log.hpp"

class ccl_base_thread
{
public:
    ccl_base_thread() = delete;
    ccl_base_thread(const ccl_base_thread& other) = delete;
    ccl_base_thread& operator= (const ccl_base_thread& other) = delete;
    ccl_base_thread(size_t idx, void*(*progress_function)(void*)) :
                    idx(idx), progress_function(progress_function)
    { }
    ccl_status_t start();
    ccl_status_t stop();
    ccl_status_t pin(int proc_id);
    size_t get_idx() { return idx; }
    virtual void* get_this() { return static_cast<void*>(this); };
    
    virtual const std::string& name() const
    {
        static const std::string name("base_thread");
        return name;
    };

    const size_t idx;
    pthread_t thread{};

    void*(*progress_function)(void*);
};
