#include "native_device_api/compiler_ccl_wrappers_dispatcher.hpp"
// test case data
namespace native_api_test {
const size_t tiles_count = 2;

// ADD TODO serialize/deserialize test!!!!

TEST_F(allreduce_one_device_local_fixture, subdevice_2_tiles_test) {
    using namespace native;

    auto drv_it = local_platform->drivers.find(0);
    UT_ASSERT(drv_it != local_platform->drivers.end(), "Driver by idx 0 must exist!");
    auto driver = drv_it->second;

    UT_ASSERT(driver->devices.size() == local_affinity.size(),
              "Count: %" << driver->devices.size() << ", bits: " << local_affinity.size()
                         << "Device count is not equal to affinity mask!");

    ccl_device_driver::device_ptr one_device = driver->devices.begin()->second;
    auto &subdevices = one_device->get_subdevices();

    UT_ASSERT(subdevices.size() != 0, "Empty subdevices count");

    {
        output << "Type & ID test" << std::endl;
        UT_ASSERT(one_device->get_device_properties().isSubdevice == false,
                  "Should not be subdevice. Properties:\n"
                      << one_device->to_string());

        auto device_id = one_device->get_device_id();
        UT_ASSERT(one_device->get_device_properties().deviceId == device_id,
                  "DeviceId is failed. Properties:\n"
                      << one_device->to_string());
        for (auto &subdev : subdevices) {
            UT_ASSERT(subdev.second->get_device_properties().isSubdevice == true,
                      "Must be subdevice. Properties:\n"
                          << subdev.second->to_string());
            auto subdevice_id = one_device->get_device_id();
            UT_ASSERT(one_device->get_device_properties().subdeviceId == subdevice_id,
                      "Sub DeviceId is failed. Properties:\n"
                          << one_device->to_string());
        }
    }
    {
        output << "Parent test" << std::endl;
        std::shared_ptr<ccl_device> parent_device = one_device->shared_from_this();
        for (auto &subdev : subdevices) {
            auto subdevice_parent = subdev.second->parent_device.lock();
            UT_ASSERT(subdevice_parent, "Subdevice parent is empty");
            UT_ASSERT(subdevice_parent.get() == parent_device.get(), "Parent are not equals");
        }
    }
    {
        output << "Memory test" << std::endl;
        auto device_mem = one_device->alloc_memory<int>(1024, sizeof(int));
        UT_ASSERT(device_mem.get_owner().lock().get() == one_device.get(),
                  "Device memory should have one_device parent");
        for (auto &subdev : subdevices) {
            auto subdevice_mem = subdev.second->alloc_memory<int>(1024, sizeof(int));
            UT_ASSERT(device_mem.get() != subdevice_mem.get(), "Handles should be different");
            UT_ASSERT(device_mem.get_owner().lock().get() != subdevice_mem.get_owner().lock().get(),
                      "Mem owners should be different");
            UT_ASSERT(subdevice_mem.get_owner().lock().get() == subdev.second.get(),
                      "Sub device mem do not belongto subdevice ");
            std::shared_ptr<ccl_subdevice> subdevice =
                std::dynamic_pointer_cast<ccl_subdevice>(subdevice_mem.get_owner().lock());
            UT_ASSERT(subdevice->parent_device.lock().get() == one_device.get(),
                      "Sub device mem has not parent of one_device parent");
        }
    }
}
} // namespace native_api_test
