#pragma once

#include "../base_fixture.hpp"

template <class DType>
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

template <class DType>
class ring_reduce_multi_device_fixture : public common_fixture {
protected:
    ring_reduce_multi_device_fixture()
            : common_fixture(get_global_device_indices() /*"[0:0],[0:1]"*/) {}

    ~ring_reduce_multi_device_fixture() {}

    void SetUp() override {
        create_global_platform();
        local_affinity = global_affinities.at(0);
        create_local_platform();
        create_module_descr("kernels/ring_allreduce.spv");
    }

    void TearDown() override {}
};

template <class DType>
class ring_reduce_single_device_multi_tile_fixture : public common_fixture {
protected:
    ring_reduce_single_device_multi_tile_fixture()
            : common_fixture(get_global_device_indices() /*"[0:0:0],[0:0:1]"*/) {}

    ~ring_reduce_single_device_multi_tile_fixture() {}

    void SetUp() override {
        create_global_platform();
        const auto first_node_affinity = global_affinities.at(0);
        local_affinity = first_node_affinity;
        const auto second_node_affinity = global_affinities.at(1);
        local_affinity.insert(second_node_affinity.begin(), second_node_affinity.end());
        create_local_platform();
        create_module_descr("kernels/ring_allreduce.spv");
    }

    void TearDown() override {}
};