#pragma once

#include "../base_fixture.hpp"

template <class T>
class ring_bcast_single_device_fixture : public common_fixture {
protected:
    ring_bcast_single_device_fixture() : common_fixture(get_global_device_indices() /*"[0:0]"*/) {}

    ~ring_bcast_single_device_fixture() {}

    void SetUp() override {
        create_global_platform();
        local_affinity = global_affinities.at(0);
        create_local_platform();
        create_module_descr("kernels/ring_bcast.spv");
    }

    void TearDown() override {}
};

class ring_bcast_multi_device_fixture : public common_fixture {
protected:
    ring_bcast_multi_device_fixture()
            : common_fixture(get_global_device_indices() /*"[0:0],[0:1]"*/) {}

    ~ring_bcast_multi_device_fixture() {}

    void SetUp() override {
        create_global_platform();
        local_affinity = global_affinities.at(0);
        create_local_platform();
        create_module_descr("kernels/ring_bcast.spv");
    }

    void TearDown() override {}
};

class ring_bcast_single_device_multi_tile_fixture : public common_fixture {
protected:
    ring_bcast_single_device_multi_tile_fixture()
            : common_fixture(get_global_device_indices() /*"[0:0:0],[0:0:1]"*/) {}

    ~ring_bcast_single_device_multi_tile_fixture() {}

    void SetUp() override {
        create_global_platform();
        const auto first_node_affinity = global_affinities.at(0);
        local_affinity = first_node_affinity;
        const auto second_node_affinity = global_affinities.at(1);
        local_affinity.insert(second_node_affinity.begin(), second_node_affinity.end());
        create_local_platform();
        create_module_descr("kernels/ring_bcast.spv");
    }

    void TearDown() override {}
};

template <class T>
class a2a_bcast_single_device_fixture : public common_fixture {
protected:
    a2a_bcast_single_device_fixture() : common_fixture(get_global_device_indices() /*"[0:0]"*/) {}

    ~a2a_bcast_single_device_fixture() {}

    void SetUp() override {
        create_global_platform();
        local_affinity = global_affinities.at(0);
        create_local_platform();
        create_module_descr("kernels/a2a_bcast.spv");
    }

    void TearDown() override {}
};

class a2a_bcast_multi_device_fixture : public common_fixture {
protected:
    a2a_bcast_multi_device_fixture()
            : common_fixture(get_global_device_indices() /*"[0:0],[0:1]"*/) {}

    ~a2a_bcast_multi_device_fixture() {}

    void SetUp() override {
        create_global_platform();
        local_affinity = global_affinities.at(0);
        create_local_platform();
        create_module_descr("kernels/a2a_bcast.spv");
    }

    void TearDown() override {}
};

class a2a_bcast_single_device_multi_tile_fixture : public common_fixture {
protected:
    a2a_bcast_single_device_multi_tile_fixture()
            : common_fixture(get_global_device_indices() /*"[0:0:0],[0:0:1]"*/) {}

    ~a2a_bcast_single_device_multi_tile_fixture() {}

    void SetUp() override {
        create_global_platform();
        const auto first_node_affinity = global_affinities.at(0);
        local_affinity = first_node_affinity;

        const auto second_node_affinity = global_affinities.at(1);
        local_affinity.insert(second_node_affinity.begin(), second_node_affinity.end());
        create_local_platform();
        create_module_descr("kernels/a2a_bcast.spv");
    }

    void TearDown() override {}
};
