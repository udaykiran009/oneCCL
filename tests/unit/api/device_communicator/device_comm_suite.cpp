#include <gtest/gtest.h>

#include "ccl_config.h"

#ifdef CCL_ENABLE_SYCL
    #include "device_comm_cases_sycl.hpp"
#else
    #include "device_comm_cases.hpp"
#endif

int main(int ac, char* av[])
{
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
