#pragma once

#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "common/event/impls/event_impl.hpp"

namespace ccl {

// event returned by stub backend
class stub_event_impl final : public event_impl {
public:
    stub_event_impl() = default;

    void wait() override {}

    bool test() override {
        return true;
    }

    bool cancel() override {
        return true;
    }

    event::native_t& get_native() override {
        throw ccl::exception(std::string(__FUNCTION__) + " - no native event for stub event");
    }

    ~stub_event_impl() override = default;
};

} // namespace ccl
