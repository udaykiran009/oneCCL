#include <gtest/gtest.h>

#include "oneapi/ccl/config.h"

#ifdef CCL_ENABLE_SYCL
#include "stream_cases_sycl.hpp"
#else
#ifdef MULTI_GPU_SUPPORT
#include "stream_cases_l0.hpp"
#else
#include "stream_cases_empty.hpp"
#endif
#endif

int main(int ac, char* av[]) {
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
