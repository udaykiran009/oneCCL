#pragma once
#include "stream_impl.hpp"


namespace ccl
{
/* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
template <class native_stream_type,
          class T>
stream create_stream(native_stream_type& native_stream)
{
    ccl_version_t ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    return stream(stream_provider_dispatcher::create(native_stream, ret));
}
}
