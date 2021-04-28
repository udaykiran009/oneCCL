#include "base_test.hpp"
#include "export_configuration.hpp"
#include "event_pool_test.hpp"
#include "event_test.hpp"
#include <iostream>
#include <string.h>

int main(int ac, char* av[]) {
    set_test_device_indices(getenv(ut_device_affinity_mask_name));

    if (ac == 2 && strcmp(av[1], "--dump_table") == 0) {
        if (setenv("NATIVE_API_ARGS", "dump_table", 0)) {
            std::cout << "Unable to set environment variable NATIVE_API_ARGS" << std::endl;
            return 1;
        }
    }

#ifndef STANDALONE_UT
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
#else
    {
        using namespace native_api_test;
        Test_platform_info t;
        t.start();
    }
    return 0;
#endif
}
