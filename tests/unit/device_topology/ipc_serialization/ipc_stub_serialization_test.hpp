#pragma once
#include "multi_process_fixture.hpp"
#include "../topology/stubs/stub_platform.hpp"

namespace ipc_suite {

using ipc_native_type = int;

TEST_F(default_fixture, ipc_serialize_test_device) {
    ccl::device_index_type dev_idx_0(0, 2, ccl::unused_index_value);
    ccl::device_index_type dev_idx_1(0, 3, ccl::unused_index_value);
    ccl::device_index_type dev_idx_2(0, 4, ccl::unused_index_value);

    stub::make_stub_devices({ dev_idx_0, dev_idx_1, dev_idx_2 });

    native::ccl_device_platform& platform = native::get_platform();
    std::shared_ptr<stub::test_device> subdevice_0 = std::dynamic_pointer_cast<stub::test_device>(
        platform.get_device(dev_idx_0)); //std::shared_ptr<ccl_device>;

    // allocate device memory for first device
    std::shared_ptr<native::ccl_context> ctx;
    auto mem_send = subdevice_0->alloc_memory<ipc_native_type>(1024, sizeof(ipc_native_type), ctx);

    // create ipc handle from memory handles
    auto ipc_mem_send = subdevice_0->create_ipc_memory_handle(mem_send.handle, ctx);

    size_t no_offset = 0;
    std::vector<uint8_t> serialized_raw_no_offset;
    size_t serialized_bytes = ipc_mem_send.serialize(serialized_raw_no_offset, no_offset);
    UT_ASSERT(serialized_raw_no_offset.size() == serialized_bytes + no_offset,
              "Serialized bytes count is unexpected");

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

    std::shared_ptr<native::ccl_device_platform> cloned_platform;
    auto recv_ipc_handle = stub::test_device::device_ipc_memory_handle::deserialize<
        stub::test_device::device_ipc_memory_handle>(
        &data_start_ptr, recv_data_size, ctx, cloned_platform);

    UT_ASSERT(platform.get_pid() == cloned_platform->get_pid(),
              "Restored platform should be the same PID as source");

    auto owner_device = recv_ipc_handle->get_owner().lock();
    UT_ASSERT(owner_device->handle == subdevice_0->handle, "Deserialized owner device is invalid");
    UT_ASSERT(owner_device->is_subdevice() == false, "Deserialized owner device is invalid");

    //with offset
    const uint8_t* data_start_ptr2 = serialized_raw_custom_offset.data() + custom_offset;
    size_t recv_data_size2 = serialized_raw_custom_offset.size();

    recv_ipc_handle = stub::test_device::device_ipc_memory_handle::deserialize<
        stub::test_device::device_ipc_memory_handle>(
        &data_start_ptr2, recv_data_size2, ctx, cloned_platform);

    UT_ASSERT(platform.get_pid() == cloned_platform->get_pid(),
              "Restored platform should be the same PID as source");

    owner_device = recv_ipc_handle->get_owner().lock();
    UT_ASSERT(owner_device->handle == subdevice_0->handle, "Deserialized owner device is invalid");
    UT_ASSERT(owner_device->is_subdevice() == false, "Deserialized owner device is invalid");
}

TEST_F(default_fixture, ipc_serialize_test_subdevice) {
    ccl::device_index_type subdev_idx_0(0, 2, 3);
    ccl::device_index_type subdev_idx_1(0, 2, 4);
    ccl::device_index_type subdev_idx_2(0, 3, 3);

    stub::make_stub_devices({ subdev_idx_0, subdev_idx_1, subdev_idx_2 });

    native::ccl_device_platform& platform = native::get_platform();
    std::shared_ptr<stub::test_subdevice> subdevice_0 =
        std::dynamic_pointer_cast<stub::test_subdevice>(
            platform.get_device(subdev_idx_0)); //std::shared_ptr<ccl_device>;

    // allocate device memory for first device
    std::shared_ptr<native::ccl_context> ctx;
    auto mem_send = subdevice_0->alloc_memory<ipc_native_type>(1024, sizeof(ipc_native_type), ctx);

    // create ipc handle from memory handles
    auto ipc_mem_send = subdevice_0->create_ipc_memory_handle(mem_send.handle, ctx);

    size_t no_offset = 0;
    std::vector<uint8_t> serialized_raw_no_offset;
    size_t serialized_bytes = ipc_mem_send.serialize(serialized_raw_no_offset, no_offset);
    UT_ASSERT(serialized_raw_no_offset.size() == serialized_bytes + no_offset,
              "Serialized bytes count is unexpected");

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

    std::shared_ptr<native::ccl_device_platform> cloned_platform;
    auto recv_ipc_handle = stub::test_subdevice::device_ipc_memory_handle::deserialize<
        stub::test_subdevice::device_ipc_memory_handle>(
        &data_start_ptr, recv_data_size, ctx, cloned_platform);

    UT_ASSERT(platform.get_pid() == cloned_platform->get_pid(),
              "Restored platform should be the same PID as source");

    auto owner_device = recv_ipc_handle->get_owner().lock();
    UT_ASSERT(owner_device->handle == subdevice_0->handle, "Deserialized owner device is invalid");
    UT_ASSERT(owner_device->is_subdevice() == true, "Deserialized owner device is invalid");

    //with offset
    const uint8_t* data_start_ptr2 = serialized_raw_custom_offset.data() + custom_offset;
    size_t recv_data_size2 = serialized_raw_custom_offset.size();

    recv_ipc_handle = stub::test_subdevice::device_ipc_memory_handle::deserialize<
        stub::test_subdevice::device_ipc_memory_handle>(
        &data_start_ptr2, recv_data_size2, ctx, cloned_platform);

    UT_ASSERT(platform.get_pid() == cloned_platform->get_pid(),
              "Restored platform should be the same PID as source");

    owner_device = recv_ipc_handle->get_owner().lock();
    UT_ASSERT(owner_device->handle == subdevice_0->handle, "Deserialized owner device is invalid");
    UT_ASSERT(owner_device->is_subdevice() == true, "Deserialized owner device is invalid");
}

} // namespace ipc_suite
