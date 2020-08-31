#include "base.hpp"
#include "fixture.hpp"
#include "kernels/ring_allgatherv_single_device_test.hpp"
#include "kernels/ring_allreduce_single_device_test.hpp"
#include "kernels/ring_alltoallv_single_device_test.hpp"
#include "kernels/ring_bcast_single_device_test.hpp"
#include "kernels/ring_reduce_single_device_test.hpp"
#include "kernels/ring_reduce_scatter_single_device_test.hpp"

int main(int ac, char* av[]) {
    set_test_device_indices(getenv("L0_CLUSTER_AFFINITY_MASK"));
#ifndef STANDALONE_UT
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
#else

    using namespace ring_single_device_case;
    {
        Test_ring_allgathrerv_single_device_mt t;
        t.start();
    }

    {
        Test_ring_allreduce_single_device_mt t;
        t.start();
    }

    {
        Test_ring_bcast_single_device_mt t;
        t.start();
    }

    {
        Test_ring_reduce_single_device_mt t;
        t.start();
    }

    {
        Test_ring_reduce_scatter_single_device_mt t;
        t.start();
    }
    {
        Test_ring_alltoallv_single_device_mt t;
        t.start();
    }
    return 0;
#endif
}
