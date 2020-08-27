#include "base.hpp"
#include "fixture.hpp"
#include "kernels/ring_allreduce_multi_device_test.hpp"
#include "kernels/ring_bcast_multi_device_test.hpp"

int main(int ac, char* av[]) {
    set_test_device_indices(getenv("L0_CLUSTER_AFFINITY_MASK"));
#ifndef STANDALONE_UT
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
#else

    using namespace ring_multi_device_case;
    {
        Test_ring_allreduce_multi_device_mt t;
        t.start();
    }
    {
        Test_ring_bcast_multi_device_mt t;
        t.start();
    }
    {
        Test_ipc_memory_test t;
        t.start();
    }
    {
        Test_multithreading_kernel_execution t;
        t.start();
    }
    return 0;
#endif
}
