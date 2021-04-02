#pragma once
#include "common_platform_fixture.hpp"
#include "common/comm/l0/context/scale/base/base_session_key.hpp"
#include "common/comm/l0/context/scale/numa/numa_session.hpp"
#include "common/comm/l0/modules/kernel_params.hpp"

class scale_out_fixture : public platform_fixture {
protected:
    scale_out_fixture() : platform_fixture(get_global_device_indices()) {}
    ~scale_out_fixture() = default;

    void SetUp() override {
        platform_fixture::SetUp();

        // get affinity for the first (only first process) in affinity mask
        const auto& local_affinity = cluster_device_indices[0];
        create_local_platform(local_affinity);
    }
};