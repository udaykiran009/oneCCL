#include "base_test.hpp"
#include "ipc_serialize_test.hpp"

int main(int ac, char* av[]) {
    set_test_device_indices(getenv(ut_device_affinity_mask_name));
#ifndef STANDALONE_UT
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
#else
    {}
#endif
}
