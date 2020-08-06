#include "base.hpp"
#include "fixture.hpp"
#include "kernels/allreduce_test.hpp"
#include "kernels/bcast_test.hpp"
#include "kernels/reduce_test.hpp"

int main(int ac, char* av[]) {
    set_test_device_indices(getenv("L0_CLUSTER_AFFINITY_MASK"));
#ifndef STANDALONE_UT
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
#else
    {
        using namespace singledevice_case;
        Test_allreduce_one_device_multithread_kernel t;
        t.start();
        using namespace bcast_singledevice_case;
        Test_bcast_one_device_multithread_kernel t2;
        t2.start();
        using namespace reduce_singledevice_case;
        Test_reduce_one_device_multithread_kernel t3;
        t3.start();
    }
#endif
}
