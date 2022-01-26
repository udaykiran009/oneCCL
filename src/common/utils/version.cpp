#include "oneapi/ccl/config.h"
#include "oneapi/ccl/types.hpp"
#include "common/utils/version.hpp"
#include "oneapi/ccl/native_device_api/export_api.hpp"

namespace ccl {
namespace utils {

ccl::library_version get_library_version() {
    ccl::library_version version = { CCL_MAJOR_VERSION,          CCL_MINOR_VERSION,
                                     CCL_UPDATE_VERSION,         CCL_PRODUCT_STATUS,
                                     CCL_PRODUCT_BUILD_DATE,     CCL_PRODUCT_FULL,
                                     ccl::backend_traits::name() };

    return version;
}

} // namespace utils
} // namespace ccl
