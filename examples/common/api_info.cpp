#include <stdio.h>

#include "base.h"

int main()
{
    test_init();

    ccl_version_t version;
    ccl_status_t ret = ccl_get_version(&version);
    if(ret != ccl_status_success)
    {
        perror("Cannot get CCL version\n");
        return -1;
    }

    printf("CCL library info:\nVersion:\nmajor: %d\nminor: %d\nupdate: %d\n"
                            "\nProduct: %s\nBuild date: %s\nFull: %s\n",
                            version.major, version.minor, version.update,
                            version.product_status, version.build_date, version.full);

    printf("\nCCL API info:\nVersion:\nmajor: %d\nminor: %d\nupdate: %d\n"
                        "\nProduct: %s\nBuild date: %s\nFull: %s\n",
                        CCL_MAJOR_VERSION, CCL_MINOR_VERSION, CCL_UPDATE_VERSION,
                        CCL_PRODUCT_STATUS, CCL_PRODUCT_BUILD_DATE, CCL_PRODUCT_FULL);

    if(CCL_MAJOR_VERSION == version.major && CCL_MINOR_VERSION == version.minor)
    {
        printf("API & library versions compatible\n");
    }
    else
    {
        perror("API & library versions are not compatible!\n");
        return -1;
    }

    test_finalize();

    if (rank == 0)
        printf("PASSED\n");
    return 0;
}
