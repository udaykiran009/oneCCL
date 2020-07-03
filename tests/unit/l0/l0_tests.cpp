#include "base.hpp"
#include "fixture.hpp"

#include "cases.hpp"
#include "kernel_storage.hpp"

int main(int ac, char* av[]) {
    set_test_device_indices(getenv("L0_CLUSTER_AFFINITY_MASK"));
#ifndef STANDALONE_UT
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
#else
    {
        Test_one_device_multithread_kernel t;
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
#endif
}
