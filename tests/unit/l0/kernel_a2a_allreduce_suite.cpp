#include "base.hpp"
#include "fixture.hpp"
#include "kernels/a2a_allreduce_test.hpp"
#include "kernels/a2a_bcast_test.hpp"

int main(int ac, char* av[]) {
    set_test_device_indices(getenv("L0_CLUSTER_AFFINITY_MASK"));
#ifndef STANDALONE_UT
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
#else
    {
        using namespace singledevice_case;
        Test_a2a_allreduce_one_device_multithread_kernel t;
        t.start();
        using namespace a2a_bcast_singledevice_case;
        Test_a2a_bcast_one_device_multithread_kernel t2;
        t2.start();
    }
    return 0;
#endif
}
