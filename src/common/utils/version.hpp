#pragma once

#include "oneapi/ccl/ccl_config.h"
#include "oneapi/ccl/ccl_types.hpp"

namespace utils {

inline ccl::library_version get_library_version() {
    ccl::library_version version{};

    version.major = CCL_MAJOR_VERSION;
    version.minor = CCL_MINOR_VERSION;
    version.update = CCL_UPDATE_VERSION;
    version.product_status = CCL_PRODUCT_STATUS;
    version.build_date = CCL_PRODUCT_BUILD_DATE;
    version.full = CCL_PRODUCT_FULL;

    return version;
}

} // namespace utils
