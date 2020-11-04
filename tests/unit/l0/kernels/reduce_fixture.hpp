#pragma once

#include "../base_fixture.hpp"

template <class T>
class ring_reduce_single_device_fixture : public common_fixture {
protected:
    ring_reduce_single_device_fixture() : common_fixture(get_global_device_indices() /*"[0:0]"*/) {}

    ~ring_reduce_single_device_fixture() {}

    void SetUp() override {
        create_global_platform();
        local_affinity = global_affinities.at(0);
        create_local_platform();
        create_module_descr("kernels/ring_reduce.spv");
    }

    void TearDown() override {}
};
