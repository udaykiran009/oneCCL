#pragma once
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "common/event/impls/event_impl.hpp"

namespace ccl {

class empty_event_impl final : public event_impl {
public:
    empty_event_impl() = default;

    void wait() override {}

    bool test() override {
        return true;
    }

    bool cancel() override {
        return true;
    }

    event::native_t& get_native() override {
        throw ccl::exception(std::string(__FUNCTION__) + " - no native event for empty event");
    }

    ~empty_event_impl() override = default;
};

} // namespace ccl
