#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"

#include "oneapi/ccl/ccl_kvs_attr_ids.hpp"
#include "oneapi/ccl/ccl_kvs_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_kvs_attr.hpp"

namespace ccl {

template <class attr>
CCL_API attr ccl_empty_attr::create_empty() {
    return attr{ ccl_empty_attr::version };
}

CCL_API kvs_attr default_kvs_attr = ccl_empty_attr::create_empty<kvs_attr>();

} // namespace ccl
