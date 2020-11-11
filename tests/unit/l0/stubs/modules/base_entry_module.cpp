#include <atomic>
#include <map>
#include <set>
#include <tuple>

#include "coll/coll.hpp"
#include "oneapi/ccl/native_device_api/export_api.hpp"
#include "common/comm/l0/modules/base_entry_module.hpp"
#include "oneapi/ccl/native_device_api/l0/context.hpp"

namespace native {
gpu_module_base::gpu_module_base(handle module_handle) : module(module_handle) {}

gpu_module_base::~gpu_module_base() {
    release();
}

void gpu_module_base::release() {
}

gpu_module_base::handle gpu_module_base::get() const {
    return module;
}

ze_kernel_handle_t gpu_module_base::import_kernel(const std::string& name) {
    return ze_kernel_handle_t{};
}

} // namespace native
