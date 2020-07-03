#pragma once
#include "../base_fixture.hpp"
namespace ipc_singledevice_case {
class ipc_one_dev_fixture : public ipc_fixture {
protected:
    ipc_one_dev_fixture() : ipc_fixture("[0:0],[0:0]", "kernels/ring_allreduce.spv") {}

    ~ipc_one_dev_fixture() override {}
};

} // namespace ipc_singledevice_case
