#include "common/global/global.hpp"

namespace ccl {
namespace ze {

global_data_desc::global_data_desc() {
    // tell L0 to create sysman devices.
    // *GetProperties() will fail if the sysman is not activated
    setenv("ZES_ENABLE_SYSMAN", "1", 0);

    LOG_INFO("initializing level-zero");
    ZE_CALL(zeInit, (ZE_INIT_FLAG_GPU_ONLY));

    uint32_t driver_count{};
    ZE_CALL(zeDriverGet, (&driver_count, nullptr));
    driver_list.resize(driver_count);
    ZE_CALL(zeDriverGet, (&driver_count, driver_list.data()));
    LOG_DEBUG("found drivers: ", driver_list.size());

    context_list.resize(driver_list.size());
    for (size_t i = 0; i < driver_list.size(); ++i) {
        ze_context_desc_t desc = ze::default_context_desc;
        ZE_CALL(zeContextCreate, (driver_list.at(i), &desc, &context_list.at(i)));

        uint32_t device_count{};
        ZE_CALL(zeDeviceGet, (driver_list.at(i), &device_count, nullptr));
        std::vector<ze_device_handle_t> devs(device_count);
        ZE_CALL(zeDeviceGet, (driver_list.at(i), &device_count, devs.data()));
        device_list.insert(device_list.end(), devs.begin(), devs.end());
        for (auto dev : devs) {
            uint32_t subdevice_count{};
            ZE_CALL(zeDeviceGetSubDevices, (dev, &subdevice_count, nullptr));
            std::vector<ze_device_handle_t> subdevs(subdevice_count);
            ZE_CALL(zeDeviceGetSubDevices, (dev, &subdevice_count, subdevs.data()));
            device_list.insert(device_list.end(), subdevs.begin(), subdevs.end());
        }
    }
    LOG_DEBUG("found devices: ", device_list.size());

    cache = std::make_unique<ze::cache>(global_data::env().worker_count);

    timer_printer = std::make_unique<kernel_timer_printer>();

    LOG_INFO("initialized level-zero");
}

global_data_desc::~global_data_desc() {
    LOG_INFO("finalizing level-zero");
    cache.reset();

    for (auto& context : context_list) {
        ZE_CALL(zeContextDestroy, (context));
    }
    context_list.clear();
    device_list.clear();
    driver_list.clear();

#if defined(CCL_ENABLE_SYCL)
    timer_printer.reset();
#endif // CCL_ENABLE_SYCL

    LOG_INFO("finalized level-zero");
}

} // namespace ze
} // namespace ccl
