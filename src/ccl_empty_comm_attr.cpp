#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"

#include "oneapi/ccl/ccl_comm_attr_ids.hpp"
#include "oneapi/ccl/ccl_comm_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_comm_attr.hpp"

namespace ccl {

template <class attr>
CCL_API attr ccl_empty_attr::create_empty() {
    return attr{ ccl_empty_attr::version };
}

CCL_API comm_attr default_comm_attr = ccl_empty_attr::create_empty<comm_attr>();

} // namespace ccl
