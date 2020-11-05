#pragma once
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "common/event/impls/event_impl.hpp"
#include "common/event/ccl_event.hpp"

namespace ccl {

class native_event_impl final : public event_impl {
public:
    explicit native_event_impl(std::unique_ptr<ccl_event> ev);
    ~native_event_impl() override = default;

    void wait() override;
    bool test() override;
    bool cancel() override;
    event::native_t& get_native() override;

private:
    std::unique_ptr<ccl_event> ev = nullptr;
    bool completed = false;
};

} // namespace ccl
