#pragma once
// #include "ccl.hpp"

class ccl_request;

namespace ccl {
class host_request_impl final : public request {
public:
    explicit host_request_impl(ccl_request* r);
    ~host_request_impl() override;

    void wait() override;
    bool test() override;

private:
    ccl_request* req = nullptr;
    bool completed = false;
};
} // namespace ccl
