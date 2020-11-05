#include <gtest/gtest.h>

#include "oneapi/ccl/config.h"

#ifdef MULTI_GPU_SUPPORT
#ifdef CCL_ENABLE_SYCL
#include "communicator_cases_sycl.hpp"
#else
#include "communicator_cases.hpp"
#endif
#endif

int main(int ac, char* av[]) {
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
