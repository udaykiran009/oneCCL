#include <gtest/gtest.h>

#include "oneapi/ccl/ccl_config.h"

#ifdef CCL_ENABLE_SYCL
    #include "stream_cases_sycl.hpp"
#else
    #include "stream_cases.hpp"
#endif

int main(int ac, char* av[])
{
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
