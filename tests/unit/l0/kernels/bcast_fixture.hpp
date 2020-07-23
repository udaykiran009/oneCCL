#pragma once
#include "../base_fixture.hpp"
namespace bcast_singledevice_case {
class bcast_one_device_local_fixture : public common_fixture {
protected:
    bcast_one_device_local_fixture() : common_fixture(get_global_device_indices() /*"[0:0]"*/) {}

    ~bcast_one_device_local_fixture() {}

    void SetUp() override {
        create_global_platform();
        local_affinity = global_affinities.at(0);
        create_local_platform();
        create_module_descr("kernels/bcast.spv");
    }

    void TearDown() override {}
};

class bcast_multi_device_local_fixture : public common_fixture {
protected:
    bcast_multi_device_local_fixture()
            : common_fixture(get_global_device_indices() /*"[0:0],[0:1]"*/) {}

    ~bcast_multi_device_local_fixture() {}

    void SetUp() override {
        create_global_platform();
        local_affinity = global_affinities.at(0);
        create_local_platform();
        create_module_descr("kernels/bcast.spv");
    }

    void TearDown() override {}
};

class bcast_one_device_multi_tile_local_fixture : public common_fixture {
protected:
    bcast_one_device_multi_tile_local_fixture()
            : common_fixture(get_global_device_indices() /*"[0:0:0],[0:0:1]"*/) {}

    ~bcast_one_device_multi_tile_local_fixture() {}

    void SetUp() override {
        create_global_platform();
        const auto first_node_affinity = global_affinities.at(0);
        local_affinity = first_node_affinity;

        const auto second_node_affinity = global_affinities.at(1);
        local_affinity.insert(second_node_affinity.begin(), second_node_affinity.end());
        create_local_platform();
        create_module_descr("kernels/bcast.spv");
    }

    void TearDown() override {}
};

} // namespace bcast_singledevice_case
