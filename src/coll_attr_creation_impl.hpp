#pragma once
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/coll_attr.hpp"
#include "coll/coll_attributes.hpp"
#include "common/utils/version.hpp"

namespace ccl {

namespace v1 {

/* TODO temporary function for UT compilation: would be part of ccl::detail::environment in final*/
template <class coll_attribute_type, class... attr_val_type>
coll_attribute_type create_coll_attr(attr_val_type&&... avs) {
    auto version = utils::get_library_version();
    auto coll_attr = coll_attribute_type(version);

    int expander[]{ (coll_attr.template set<attr_val_type::idx()>(avs.val()), 0)... };
    (void)expander;
    return coll_attr;
}

} // namespace v1

} // namespace ccl
