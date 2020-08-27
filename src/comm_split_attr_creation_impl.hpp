#pragma once
#include "ccl_types.hpp"
#include "ccl_comm_split_attr.hpp"

namespace ccl
{
/* TODO temporary function for UT compilation: would be part of ccl::environment in final*/

template <class attr_t, class ...attr_value_pair_t>
static attr_t create_attr(attr_value_pair_t&&...avps)
{
    ccl_version_t ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    auto attr = attr_t(ret);

    int expander [] {(attr.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
    return attr;
}

template <class ...attr_value_pair_t>
comm_split_attr_t create_comm_split_attr(attr_value_pair_t&&...avps)
{
    return create_attr<comm_split_attr_t>(std::forward<attr_value_pair_t>(avps)...);
}

#ifdef MULTI_GPU_SUPPORT

template <class ...attr_value_pair_t>
device_comm_split_attr_t create_device_comm_split_attr(attr_value_pair_t&&...avps)
{
    return create_attr<device_comm_split_attr_t>(std::forward<attr_value_pair_t>(avps)...);
}

#endif

}
