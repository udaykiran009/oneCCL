#pragma once
#include "multi_process_fixture.hpp"
#include "common/comm/l0/devices/communication_structs/ipc_server.hpp"
#include "common/comm/l0/devices/communication_structs/ipc_connection.hpp"

namespace ipc_suite {

using native_type = int;

TEST_F(multi_platform_fixture, ipc_unix_server_handles_serialize) {
    using namespace native;
    using namespace net;

    std::shared_ptr<ccl_context> ctx;

    // check global driver
    constexpr size_t l0_driver_index = 0;
    auto drv_it = global_platform->drivers.find(l0_driver_index);
    UT_ASSERT(drv_it != global_platform->drivers.end(), "Global driver by idx 0 must exist!");
    ccl_device_driver& global_driver = *drv_it->second;

    // check local driver per process
    drv_it = local_platform->drivers.find(l0_driver_index);
    UT_ASSERT(drv_it != local_platform->drivers.end(), "Driver by idx 0 must exist!");
    ccl_device_driver& driver = *drv_it->second;

    // check devices per process
    UT_ASSERT(driver.devices.size() > 1,
              "IPC test scope require at least 2 devices in local platform! Use correct \""
                  << ut_device_affinity_mask_name << "\"");

    // initialize device memory
    using mem_handles_container = std::vector<ccl_device::device_memory<native_type>>;
    std::map<ccl_device*, mem_handles_container> device_allocated_memory_storage;

    constexpr size_t data_size = 32;
    std::vector<native_type> send_values(data_size, *my_pid);
    std::vector<native_type> recv_values(data_size, *other_pid);

    // allocate device memory
    output << "PID: " << *my_pid << " allocate device memory" << std::endl;
    for (auto& dev_it : driver.devices) {
        ccl_device& dev = *dev_it.second;
        try {
            auto mem_send = dev.alloc_memory<native_type>(data_size, sizeof(native_type), ctx);
            auto mem_recv = dev.alloc_memory<native_type>(data_size, sizeof(native_type), ctx);

            mem_send.enqueue_write_sync(send_values);
            mem_recv.enqueue_write_sync(recv_values);

            // remember for further check
            mem_handles_container& handles = device_allocated_memory_storage[&dev];
            handles.push_back(std::move(mem_send));
            handles.push_back(std::move(mem_recv));
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false,
                      "Cannot allocate memory for device: " << dev.to_string()
                                                            << "\nError: " << ex.what());
        }
    }

    // create ipc handle from memory handles
    output << "PID: " << *my_pid << " create ipc handle from memory handles" << std::endl;
    using ipc_handle_container = std::vector<ccl_device::device_ipc_memory_handle>;
    std::map<ccl_device*, ipc_handle_container> ipc_mem_storage_to_send;
    using ipc_handles_ptr_container =
        std::vector<std::shared_ptr<ccl_device::device_ipc_memory_handle>>;
    std::map<ccl_device*, ipc_handles_ptr_container> ipc_mem_storage_to_receive;
    for (auto& device_storage : device_allocated_memory_storage) {
        // get device with its memory handles
        ccl_device* device_ptr = device_storage.first;
        try {
            mem_handles_container& memory_handles = device_storage.second;

            // prepare send handles and receive handles storage
            ipc_handle_container& out_ipc_handles = ipc_mem_storage_to_send[device_ptr];
            out_ipc_handles.reserve(memory_handles.size());

            ipc_handles_ptr_container& in_ipc_handles = ipc_mem_storage_to_receive[device_ptr];
            in_ipc_handles.resize(memory_handles.size());

            // create ic handles to send it later
            std::transform(
                memory_handles.begin(),
                memory_handles.end(),
                std::back_inserter(out_ipc_handles),
                [device_ptr, ctx](ccl_device::device_memory<native_type>& memory_handle) {
                    return device_ptr->create_ipc_memory_handle(memory_handle.handle, ctx);
                });
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false,
                      "Cannot create ip memory handle for device: " << device_ptr->to_string()
                                                                    << "\nError: " << ex.what());
        }
    }

    // send to other process
    ipc_server server;
    std::string addr{ "ipc_unix_suite_" };
    std::string my_addr = addr + std::to_string(*my_pid);
    std::string other_addr = addr + std::to_string(*other_pid);

    // start server
    output << "PID: " << *my_pid << " start server: " << my_addr << std::endl;
    server.start(my_addr);

    unsigned char phase = 0;
    wait_phase(phase++);
    std::unique_ptr<ipc_tx_connection> tx_conn(new ipc_tx_connection(other_addr));

    // wait incoming connection
    wait_phase(phase++);

    std::unique_ptr<ipc_rx_connection> rx_conn = server.process_connection();
    UT_ASSERT(rx_conn, "RX connection have to be accepted at moment");

    // send data
    std::map<ccl_device*, std::vector<uint8_t>> received_raw_data_per_device;
    for (const auto& device_storage : ipc_mem_storage_to_send) {
        const ipc_handle_container& ipc_mem_handles = device_storage.second;
        std::vector<uint8_t> serialized_raw_handles =
            tx_conn->send_ipc_memory(ipc_mem_handles, *my_pid);

        //prepare recv array
        received_raw_data_per_device[device_storage.first].resize(serialized_raw_handles.size());
    }

    // receive data
    for (auto& device_storage : ipc_mem_storage_to_receive) {
        ipc_handles_ptr_container& ipc_handles = device_storage.second;
        std::vector<uint8_t>& data_to_recv = received_raw_data_per_device[device_storage.first];

        size_t received_rank = 0;
        auto got = rx_conn->receive_ipc_memory(data_to_recv, received_rank);
        ipc_handles.swap(got);

        UT_ASSERT(static_cast<int>(received_rank) == *other_pid, "Received rank is invalid");
        size_t index = 0;
        for (auto& recv_ipc_handle : ipc_handles) {
            std::shared_ptr<ccl_device> owner_device = recv_ipc_handle->get_owner().lock();
            auto ipc_device_it = std::find_if(
                global_driver.devices.begin(),
                global_driver.devices.end(),
                [owner_device](typename ccl_device_driver::devices_storage_type::value_type& dev) {
                    return dev.second->handle == owner_device->handle;
                });
            UT_ASSERT(ipc_device_it != global_driver.devices.end(),
                      "Cannot find ipc device in global driver");

            output << "PID: " << *my_pid << " restore memory from IPC handle" << std::endl;
            try {
                ccl_device::device_ipc_memory recovered_mem =
                    owner_device->get_ipc_memory(std::move(recv_ipc_handle), ctx);

                std::vector<native_type> read_data(data_size, 0);
                if (index == 0) //send_mem
                {
                    native::detail::copy_memory_sync_unsafe(read_data.data(),
                                                            recovered_mem.get().pointer,
                                                            data_size * sizeof(native_type),
                                                            owner_device,
                                                            ctx);

                    std::vector<native_type> read_data_expected(data_size, *other_pid);
                    UT_ASSERT(read_data == read_data_expected,
                              "Get data for send_buf is unexpected!");
                }
                else //recv_mem
                {
                    native::detail::copy_memory_sync_unsafe(read_data.data(),
                                                            recovered_mem.get().pointer,
                                                            data_size * sizeof(native_type),
                                                            owner_device,
                                                            ctx);
                    std::vector<native_type> read_data_expected(data_size, *my_pid);
                    UT_ASSERT(read_data == read_data_expected,
                              "Get data for send_buf is unexpected!");
                }
            }
            catch (const std::exception& ex) {
                UT_ASSERT(false,
                          "Cannot restore ipc memory handle at index: " << index << ". Error: "
                                                                        << ex.what());
            }
            index++;
        }
    }
    output << "PID: " << *my_pid << " finalize process" << std::endl;
}
} // namespace ipc_suite
