#pragma once

#include "exec/thread/base_thread.hpp"

class ccl_listener : public ccl_base_thread
{
public:
    ccl_listener() = delete;
    ccl_listener(ccl_global_data*);
    ccl_listener(const ccl_listener& other) = delete;
    ccl_listener& operator= (const ccl_listener& other) = delete;
    virtual ~ccl_listener() = default;
    virtual void* get_this() override { return static_cast<void*>(this); };
    
    virtual const std::string& name() const override
    {
        static const std::string name("listener");
        return name;
    };

    ccl_global_data* gl_data;
};
