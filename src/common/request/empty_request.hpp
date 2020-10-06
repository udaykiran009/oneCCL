#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "common/request/request_impl.hpp"

namespace ccl {

class empty_request_impl final : public request_impl {
public:
    empty_request_impl() = default;

    void wait() override {

    }

    bool test() override {
        return true;
    }

    bool cancel() override {
        return true;
    }

    event::native_t& get_native() override {
        throw ccl::exception(std::string(__FUNCTION__) + " - no native event for empty event");
    }

    ~empty_request_impl() override {

    }
};

} // namespace ccl
