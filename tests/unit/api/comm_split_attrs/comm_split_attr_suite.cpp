#include <gtest/gtest.h>

#include "oneapi/ccl/ccl_config.h"

#include "host_comm_split_attr_cases.hpp"

#ifdef MULTI_GPU_SUPPORT
    #include "device_comm_split_attr_cases.hpp"
#endif

int main(int ac, char* av[])
{
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
