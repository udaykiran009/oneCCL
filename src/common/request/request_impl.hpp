#pragma once

namespace ccl {

class event;

class request_impl {
public:
    virtual void wait() = 0;
    virtual bool test() = 0;
    virtual bool cancel() = 0;
    virtual event& get_event() = 0;
    virtual ~request_impl() = default;
};

} // namespace ccl
