#include "base.hpp"
#include "fixture.hpp"
#include "kernels/ipc_single_dev_allreduce_test.hpp"

int main(int ac, char* av[])
{
#ifndef STANDALONE_UT
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
#else
    using namespace ipc_singledevice_case;
    {
        Test_ring_allreduce_one_device t;
        t.start();
    }
#endif
}

