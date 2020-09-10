#include "oneapi/ccl/ccl_types.hpp"

namespace ccl
{
ccl::version ccl_empty_attr::version {
    CCL_MAJOR_VERSION,
    CCL_MINOR_VERSION,
    CCL_UPDATE_VERSION,
    CCL_PRODUCT_STATUS,
    CCL_PRODUCT_BUILD_DATE,
    CCL_PRODUCT_FULL,
};

template<class attr>
attr ccl_empty_attr::create_empty()
{
    return attr{ccl_empty_attr::version};
}
}
