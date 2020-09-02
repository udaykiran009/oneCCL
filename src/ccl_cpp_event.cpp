#include "event_impl.hpp"


namespace ccl
{

CCL_API event::event(event&& src) :
        base_t(std::move(src))
{
}

CCL_API event::event(impl_value_t&& impl) :
        base_t(std::move(impl))
{
}

CCL_API event::~event()
{
}

CCL_API event& event::operator=(event&& src)
{
    if (src.get_impl() != this->get_impl())
    {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}

void event::build_from_params()
{
    get_impl()->build_from_params();
}
}
