#include "ccl_types.hpp"
#include "ccl_request.hpp"

namespace ccl
{

void request::wait()
{
}

bool request::test()
{
    return true;
}

bool request::cancel()
{
    return true;
}

event& get_event()
{
    //todo
    static event *ret = nullptr;
    return *ret;
}
}
