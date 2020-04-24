#include "base.hpp"
#include "fixture.hpp"
#include "kernels/allreduce_test.hpp"

int main(int ac, char* av[])
{
#ifndef STANDALONE_UT
  testing::InitGoogleTest(&ac, av);
  return RUN_ALL_TESTS();
#else
{
    using namespace singledevice_case;
    Test_allreduce_one_device_multithread_kernel t;
    t.start();
}

#endif
}

