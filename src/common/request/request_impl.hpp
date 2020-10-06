#pragma once

#include "oneapi/ccl/native_device_api/export_api.hpp"
#include "oneapi/ccl/ccl_event.hpp"

namespace ccl {

class request_impl {
public:
    virtual void wait() = 0;
    virtual bool test() = 0;
    virtual bool cancel() = 0;
    virtual event::native_t& get_native() = 0;
    virtual ~request_impl() = default;
};

} // namespace ccl
