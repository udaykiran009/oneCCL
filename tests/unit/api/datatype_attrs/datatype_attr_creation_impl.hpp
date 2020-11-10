#pragma once
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/datatype_attr.hpp"

namespace ccl {

namespace v1 {

/* TODO temporary function for UT compilation: would be part of ccl::detail::environment in final*/
template <class... attr_val_type>
datatype_attr create_datatype_attr(attr_val_type&&... avs) {
    ccl::library_version ret{};
    auto attr = datatype_attr(ret);

    int expander[]{ (attr.template set<attr_val_type::idx()>(avs.val()), 0)... };
    (void)expander;
    return attr;
}

} // namespace v1

} // namespace ccl
