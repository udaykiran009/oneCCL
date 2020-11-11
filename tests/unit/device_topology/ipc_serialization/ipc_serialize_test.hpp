#pragma once
#include "ipc_fixture.hpp"

namespace ipc_suite {

using ipc_native_type = int;
TEST_F(default_fixture, ipc_serialize_test)
{
    using namespace native;
    std::shared_ptr<ccl_context> ctx;

    // check contract
    constexpr size_t l0_driver_index = 0;
    auto drv_it = global_platform->drivers.find(l0_driver_index);
    UT_ASSERT(drv_it != global_platform->drivers.end(), "Global L0 driver must exist!");
    ccl_device_driver& global_driver = *drv_it->second;
    UT_ASSERT(!global_driver.devices.empty(), "Devices must exist for L0 driver!");

    // prepare stencil data
    std::vector<ipc_native_type> send_values(1024);
    std::iota(send_values.begin(), send_values.end(), 1);


    // allocate device memory for first device
    ccl_device& dev = *global_driver.devices.begin()->second;
    auto mem_send = dev.alloc_memory<ipc_native_type>(1024, sizeof(ipc_native_type), ctx);
    mem_send.enqueue_write_sync(send_values);

    // create ipc handle from memory handles
    ccl_device::device_ipc_memory_handle ipc_mem_send =
                dev.create_ipc_memory_handle(mem_send.handle, ctx);

    // serialize it

    //with 0 offset
    size_t no_offset = 0;

    std::vector<uint8_t> serialized_raw_no_offset;
    size_t serialized_bytes = ipc_mem_send.serialize(serialized_raw_no_offset, no_offset);
    UT_ASSERT(serialized_raw_no_offset.size() == serialized_bytes + no_offset, "Serialized bytes count is unexpected");

    //with custom offset
    size_t custom_offset = 77;

    std::vector<uint8_t> serialized_raw_custom_offset;
    serialized_bytes = ipc_mem_send.serialize(serialized_raw_custom_offset, custom_offset);
    UT_ASSERT(serialized_raw_custom_offset.size() == serialized_bytes + custom_offset,
              "Custom offset serialized bytes count is unexpected");

    // deserialize it
    //with 0 offset
    const uint8_t* data_start_ptr = serialized_raw_no_offset.data() + no_offset;
    size_t recv_data_size = serialized_raw_no_offset.size();

    std::shared_ptr<ccl_device_platform> cloned_platform;
    auto recv_ipc_handle = ccl_device::device_ipc_memory_handle::deserialize<
                ccl_device::device_ipc_memory_handle>(
                &data_start_ptr, recv_data_size, ctx, cloned_platform);

    UT_ASSERT(global_platform->get_pid() == cloned_platform->get_pid(),
              "Restored platform should be the same PID as source");

    std::shared_ptr<ccl_device> owner_device = recv_ipc_handle->get_owner().lock();
    UT_ASSERT(owner_device->handle == dev.handle,
              "Deserialized owner device is invalid");

    ccl_device::device_ipc_memory ipc_mem_recv = owner_device->get_ipc_memory(std::move(recv_ipc_handle), ctx);

    UT_ASSERT(ipc_mem_recv.handle.pointer != nullptr, "Recovered IPC failed");
}
}
