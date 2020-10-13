#include "base.hpp"
#include "fixture.hpp"
#include "kernels/ring_allreduce_single_device_multi_tile_test.hpp"

int main(int ac, char* av[]) {
    set_test_device_indices(getenv("L0_CLUSTER_AFFINITY_MASK"));
#ifndef STANDALONE_UT
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
#else

    using namespace ring_single_device_multi_tile_case;
    {
        Test_ring_allreduce_single_device_multi_tile t;
        t.start();
    }
    return 0;
#endif
}
