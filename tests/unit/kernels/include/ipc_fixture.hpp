#pragma once

#include "../base_kernel_fixture.hpp"
#include "kernel_utils.hpp"

class ring_ipc_allreduce_single_device_fixture : public ipc_fixture {
protected:
    ring_ipc_allreduce_single_device_fixture()
            : ipc_fixture(get_global_device_indices(), "kernels/ring_allreduce.spv") {}

    ~ring_ipc_allreduce_single_device_fixture() override {}
};
