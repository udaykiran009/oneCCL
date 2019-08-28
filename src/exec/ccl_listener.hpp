#pragma once

#include "ccl_thread.hpp"

class ccl_listener : public ccl_thread
{
public:
    ccl_listener() = delete;
    ccl_listener(ccl_global_data*);
    ccl_listener(const ccl_listener& other) = delete;
    ccl_listener& operator= (const ccl_listener& other) = delete;
    virtual ~ccl_listener() = default;
    virtual void* get_this() override { return static_cast<void*>(this);};
    virtual std::string name() override { return "listener";};
    atl_comm_t** get_comms();
    ccl_global_data* gl_data;
};
