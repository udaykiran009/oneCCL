#include "ring_allgatherv_single_device_test.hpp"
#include "ring_allreduce_single_device_test.hpp"
//TODO: have to fix alltoallv launch, then uncoment
//#include "ring_alltoallv_single_device_test.hpp"
#include "ring_bcast_single_device_test.hpp"
#include "ring_reduce_single_device_test.hpp"
#include "ring_reduce_scatter_single_device_test.hpp"

int main(int ac, char* av[]) {
    set_test_device_indices(getenv(ut_device_affinity_mask_name));
#ifndef STANDALONE_UT
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();

#else

    using namespace ring_single_device_case;
    // {
    //     TypedTest_ring_allgatherv_single_device_mt t;
    //     t.start();
    // }

    {
        TypedTest_ring_allreduce_single_device_mt t;
        t.start();
    }

    // {
    //     TypedTest_ring_bcast_single_device_mt t;
    //     t.start();
    // }

    {
        TypedTest_ring_reduce_single_device_mt t;
        t.start();
    }

    // {
    //     TypedTest_ring_reduce_scatter_single_device_mt t;
    //     t.start();
    // }

    // {
    //     TypedTest_ring_alltoallv_single_device_mt t;
    //     t.start();
    // }
    return 0;
#endif
}
