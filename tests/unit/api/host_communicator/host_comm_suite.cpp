#include <gtest/gtest.h>

#include "oneapi/ccl/ccl_config.h"

#include "host_comm_cases.hpp"

int main(int ac, char* av[])
{
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
