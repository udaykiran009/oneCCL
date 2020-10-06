#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "common/request/request_impl.hpp"

class ccl_request;

namespace ccl {

class host_request_impl final : public ccl::request_impl {
public:
    explicit host_request_impl(ccl_request* r);
    ~host_request_impl() override;

    void wait() override;
    bool test() override;
    bool cancel() override;
    event::native_t& get_native() override;

private:
    ccl_request* req = nullptr;
    bool completed = false;
};

} // namespace ccl
