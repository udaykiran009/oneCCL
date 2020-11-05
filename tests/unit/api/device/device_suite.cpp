#include <gtest/gtest.h>

#include "oneapi/ccl/config.h"

#ifdef CCL_ENABLE_SYCL
#include "device_cases_sycl.hpp"
#else
#include "device_cases_empty.hpp"
#endif

int main(int ac, char* av[]) {
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
