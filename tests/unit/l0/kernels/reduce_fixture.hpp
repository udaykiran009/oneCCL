#include "../base_fixture.hpp"

class reduce_one_device_local_fixture : public common_fixture {
protected:
    reduce_one_device_local_fixture() : common_fixture(get_global_device_indices() /*"[0:0]"*/) {}

    ~reduce_one_device_local_fixture() {}

    void SetUp() override {
        create_global_platform();
        local_affinity = global_affinities.at(0);
        create_local_platform();
        create_module_descr("kernels/reduce.spv");
    }

    void TearDown() override {}
};
