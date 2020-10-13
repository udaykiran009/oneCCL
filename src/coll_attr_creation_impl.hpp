#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_coll_attr.hpp"
#include "coll/coll_attributes.hpp"
#include "common/utils/version.hpp"

namespace ccl {
/* TODO temporary function for UT compilation: would be part of ccl::detail::environment in final*/
template <class coll_attribute_type, class... attr_value_pair_t>
coll_attribute_type create_coll_attr(attr_value_pair_t&&... avps) {
    auto version = utils::get_library_version();
    auto coll_attr = coll_attribute_type(version);

    int expander[]{ (coll_attr.template set<attr_value_pair_t::idx()>(avps.val()), 0)... };
    (void)expander;
    return coll_attr;
}
} // namespace ccl
