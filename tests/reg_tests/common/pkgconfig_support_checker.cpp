#include "oneapi/ccl.hpp"
#include <stdio.h>

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
using namespace cl::sycl;
using namespace cl::sycl::access;
#endif // CCL_ENABLE_SYCL

int main() {
    auto version = ccl::get_library_version();
    printf("\nCompile-time CCL library version:\nmajor: %d\nminor: %d\nupdate: %d\n"
           "Product: %s\nBuild date: %s\nFull: %s\n",
           CCL_MAJOR_VERSION,
           CCL_MINOR_VERSION,
           CCL_UPDATE_VERSION,
           CCL_PRODUCT_STATUS,
           CCL_PRODUCT_BUILD_DATE,
           CCL_PRODUCT_FULL);

    printf("\nPASSED\n");
    return 0;
}
