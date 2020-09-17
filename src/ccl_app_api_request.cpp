#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "common/request/request_impl.hpp"
#include "oneapi/ccl/ccl_request.hpp"

namespace ccl {

CCL_API request::request() noexcept : base_t(nullptr) {}
CCL_API request::request(request&& src) noexcept : base_t(std::move(src)) {}
CCL_API request::request(impl_value_t&& impl) noexcept : base_t(std::move(impl)) {}
CCL_API request::~request() noexcept {}

request& CCL_API request::operator=(request&& src) noexcept
{
    if(this->get_impl() != src.get_impl()) {
        this->get_impl() = std::move(src.get_impl());
    }
    return *this;
}

bool CCL_API request::operator==(const request& rhs) const noexcept
{
    return this->get_impl() == rhs.get_impl();
}

bool CCL_API request::operator!=(const request& rhs) const noexcept
{
    return this->get_impl() != rhs.get_impl();
}

void CCL_API request::wait()
{
    return get_impl()->wait();
}

bool CCL_API request::test()
{
    return get_impl()->test();
}

bool CCL_API request::cancel()
{
    return get_impl()->cancel();
}

event& CCL_API request::get_event()
{
    return get_impl()->get_event();
}

} // namespace ccl
