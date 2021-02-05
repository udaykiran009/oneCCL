#if 0
#include "ring_ipc_allreduce_single_device_test.hpp"

int main(int ac, char* av[]) {
    set_test_device_indices(getenv(ut_device_affinity_mask_name));
#ifndef STANDALONE_UT
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
#else
    using namespace ring_single_device_case;
    {
        Test_ring_ipc_allreduce_single_device t;
        t.start();
    }
    return 0;
#endif
}

#endif
