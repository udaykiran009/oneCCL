#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"

#include "oneapi/ccl/ccl_init_attr_ids.hpp"
#include "oneapi/ccl/ccl_init_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_init_attr.hpp"

namespace ccl {

namespace v1 {

template <class attr>
CCL_API attr ccl_empty_attr::create_empty() {
    return attr{ ccl_empty_attr::version };
}

CCL_API init_attr default_init_attr = ccl_empty_attr::create_empty<init_attr>();

} // namespace v1

} // namespace ccl
