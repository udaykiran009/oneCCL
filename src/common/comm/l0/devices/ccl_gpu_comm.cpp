#include <iostream>
#include <vector>
#include <set>
#include "common/comm/l0/devices/ccl_gpu_comm.hpp"
#include "sched/sched.hpp"
#include "sched/entry/l0/l0_entry.hpp"
#include "common/comm/l0/modules/specific_modules_source_data.hpp"

namespace native {

ccl_gpu_comm::ccl_gpu_comm(ccl_device& assigned_device, size_t idx) : base(assigned_device, idx) {
    auto queue_prop = ccl_device::get_default_queue_desc();
    queue_prop.ordinal = 0;
    (void)device.get_cmd_queue(queue_prop); //default -for execution

    //compile and load modules from all sources
    load_modules(specific_modules_source_data_storage::instance());
}

std::string ccl_gpu_comm::to_string_impl() const {
    std::string ret(name());
    ret = ret + ", comm: " + comm_to_str() +
          ", virtual count: " + std::to_string(get_virtual_gpu_count());
    return ret;
}

void ccl_gpu_comm::register_virtual_gpu(ccl_virtual_gpu_comm* gpu) {
    registered_virtual_gpu_count++;
}

std::tuple<bool, ze_module_handle_t, std::string> ccl_gpu_comm::create_module_handle(
    const ze_module_desc_t& descr,
    size_t hash) {
    std::tuple<bool, ze_module_handle_t, std::string> ret{ true, nullptr, "" };

    native::ccl_device::device_module_ptr mod;
    try {
        mod = device.create_module(descr, hash);
        std::get<1>(ret) = mod->get();
    }
    catch (const std::exception& ex) {
        std::get<0>(ret) = false;
        std::get<2>(ret) = ex.what();
    }

    return ret;
}

} // namespace native
