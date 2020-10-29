#include "oneapi/ccl/ccl_types.hpp"
#include "common/utils/version.hpp"

namespace ccl {

namespace v1 {

library_version ccl_empty_attr::version = utils::get_library_version();

template <class attr>
attr ccl_empty_attr::create_empty() {
    return attr{ ccl_empty_attr::version };
}

} // namespace v1

} // namespace ccl
