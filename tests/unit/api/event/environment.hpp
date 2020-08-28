#pragma once
#include "event_impl.hpp"


namespace ccl
{
/* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
template <class native_event_type,
          class T>
event event::create_event(native_event_type& native_event)
{
    ccl_version_t ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    return event(event::impl_value_t(new event::impl_t(native_event, ret)));
}
}
