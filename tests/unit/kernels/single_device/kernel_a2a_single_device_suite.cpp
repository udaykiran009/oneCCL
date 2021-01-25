#include "base.hpp"
#include "fixture.hpp"
#include "a2a_allreduce_single_device_test.hpp"
#include "a2a_bcast_single_device_test.hpp"

int main(int ac, char* av[]) {
    set_test_device_indices(getenv("L0_CLUSTER_AFFINITY_MASK"));
#ifndef STANDALONE_UT
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
#else

    using namespace a2a_single_device_case;
    {
        Test_a2a_allreduce_single_device_mt t;
        t.start();
    }

    {
        Test_a2a_bcast_single_device_mt t;
        t.start();
    }
    return 0;
#endif
}