#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "common/request/request_impl.hpp"
#include "common/event/ccl_event.hpp"

namespace ccl {

class native_request_impl final : public request_impl {
public:
    explicit native_request_impl(event::native_t& native_event, ccl::library_version version);
    ~native_request_impl() override;

    void wait() override;
    bool test() override;
    bool cancel() override;
    event::native_t& get_native() override;

private:
    std::unique_ptr<ccl_event> ev = nullptr;
    bool completed = false;
};

} // namespace ccl
