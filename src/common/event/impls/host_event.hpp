#pragma once
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "common/event/impls/event_impl.hpp"

class ccl_request;

namespace ccl {

// event returned by ccl_comm(i.e. native backend)
class host_event_impl final : public event_impl {
public:
    explicit host_event_impl(ccl_request* r);
    ~host_event_impl() override;

    void wait() override;
    bool test() override;
    bool cancel() override;
    event::native_t& get_native() override;

private:
    ccl_request* req = nullptr;
    bool completed = false;
};

} // namespace ccl
