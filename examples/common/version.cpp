#include <stdio.h>

#include "base.hpp"

int main()
{
    auto version = ccl::environment::instance().get_library_version();

    printf("\nCompile-time CCL library version:\nmajor: %d\nminor: %d\nupdate: %d\n"
                        "Product: %s\nBuild date: %s\nFull: %s\n",
                        CCL_MAJOR_VERSION, CCL_MINOR_VERSION, CCL_UPDATE_VERSION,
                        CCL_PRODUCT_STATUS, CCL_PRODUCT_BUILD_DATE, CCL_PRODUCT_FULL);

    printf("\nRuntime CCL library version:\nmajor: %d\nminor: %d\nupdate: %d\n"
                            "Product: %s\nBuild date: %s\nFull: %s\n",
                            version.major, version.minor, version.update,
                            version.product_status, version.build_date, version.full);

    printf("\noneCCL specification version: %s\n", ONECCL_SPEC_VERSION);

    if (CCL_MAJOR_VERSION == version.major && CCL_MINOR_VERSION <= version.minor)
    {
        printf("\nVersions are compatible\n");
    }
    else
    {
        perror("\nVersions are not compatible!\n");
        return -1;
    }

    printf("\nPASSED\n");

    return 0;
}
