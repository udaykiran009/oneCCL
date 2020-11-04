#include "base.hpp"
#include "fixture.hpp"
#include "native_api/subdevice_test.hpp"
#include "native_api/platform_test.hpp"

int main(int ac, char* av[]) {
    set_test_device_indices(getenv("L0_CLUSTER_AFFINITY_MASK"));
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
