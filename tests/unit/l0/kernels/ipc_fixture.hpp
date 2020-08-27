#pragma once

#include "../base_fixture.hpp"

class ring_ipc_allreduce_single_device_fixture : public ipc_fixture {
protected:
    ring_ipc_allreduce_single_device_fixture()
            : ipc_fixture("[0:0],[0:0]", "kernels/ring_allreduce.spv") {}

    ~ring_ipc_allreduce_single_device_fixture() override {}
};
