#pragma once
#include "ccl_types.hpp"
#include "ccl_request.hpp"

class ccl_request;

namespace ccl
{
class event;
class host_request_impl final : public ccl::request
{
public:
    explicit host_request_impl(ccl_request* r);
    ~host_request_impl() override;

    void wait() override;
    bool test() override;
    bool cancel() override;
    event& get_event() override;
private:
    ccl_request* req = nullptr;
    bool completed = false;
};
}
