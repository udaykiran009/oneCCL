#include <gtest/gtest.h>

#include "oneapi/ccl/config.h"

#include "datatype_attr_cases.hpp"

int main(int ac, char* av[]) {
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
