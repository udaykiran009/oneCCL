#pragma once
#include "ccl_types.hpp"
#include "ccl_datatype_attr.hpp"

namespace ccl
{

/* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
template <class ...attr_value_pair_t>
datatype_attr_t create_datatype_attr(attr_value_pair_t&&...avps)
{
    ccl_version_t ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    auto datatype_attr = datatype_attr_t(ret);

    int expander [] {(datatype_attr.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
    (void)expander;
    return datatype_attr;
}

}
