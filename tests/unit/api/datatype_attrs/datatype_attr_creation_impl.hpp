#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_datatype_attr.hpp"

namespace ccl {

namespace v1 {

/* TODO temporary function for UT compilation: would be part of ccl::detail::environment in final*/
template <class... attr_value_pair_t>
datatype_attr create_datatype_attr(attr_value_pair_t&&... avps) {
    ccl::library_version ret{};
    auto attr = datatype_attr(ret);

    int expander[]{ (attr.template set<attr_value_pair_t::idx()>(avps.val()), 0)... };
    (void)expander;
    return attr;
}

} // namespace v1

} // namespace ccl
