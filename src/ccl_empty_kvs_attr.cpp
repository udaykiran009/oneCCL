#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/aliases.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "oneapi/ccl/type_traits.hpp"

#include "oneapi/ccl/kvs_attr_ids.hpp"
#include "oneapi/ccl/kvs_attr_ids_traits.hpp"
#include "oneapi/ccl/kvs_attr.hpp"

namespace ccl {

namespace v1 {

template <class attr>
CCL_API attr ccl_empty_attr::create_empty() {
    return attr{ ccl_empty_attr::version };
}

CCL_API kvs_attr default_kvs_attr = ccl_empty_attr::create_empty<kvs_attr>();

} // namespace v1

} // namespace ccl
