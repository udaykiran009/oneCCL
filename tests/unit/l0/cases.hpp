

TEST_F(ipc_fixture, DISABLED_ipc_memory_test)
{
    using namespace native;

    // check global driver
    auto drv_it = global_platform->drivers.find(0);
    UT_ASSERT(drv_it != global_platform->drivers.end(), "Global driver by idx 0 must exist!");
    ccl_device_driver& global_driver = *drv_it->second;

    // check local driver per process
    drv_it = local_platform->drivers.find(0);
    UT_ASSERT(drv_it != local_platform->drivers.end(), "Driver by idx 0 must exist!");
    ccl_device_driver& driver = *drv_it->second;

    // check devices per process
    UT_ASSERT(driver.devices.size() == local_affinity.size(),
             "Device count is not equal to affinity mask!"<<  driver.devices.size() << local_affinity.size());

    // initialize device memory
    using mem_handles_container = std::vector<ccl_device::device_memory<int>>;
    std::map<ccl_device*, mem_handles_container> device_allocated_memory_storage;
    std::vector<int> send_values(1024);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<int> recv_values(1024, 0);

    // allocate device memory
    output << "PID: " << pid <<  " allocate device memory" << std::endl;
    for(auto &dev_it : driver.devices)
    {
        ccl_device& dev = *dev_it.second;
        try
        {
            auto mem_send = dev.alloc_memory<int>(1024, sizeof(int));
            auto mem_recv = dev.alloc_memory<int>(1024, sizeof(int));

            mem_send.enqueue_write_sync(send_values);
            mem_recv.enqueue_write_sync(recv_values);

            // remember for further check
            mem_handles_container& handles = device_allocated_memory_storage[&dev];
            handles.push_back(std::move(mem_send));
            handles.push_back(std::move(mem_recv));
        }
        catch (const std::exception& ex)
        {
            UT_ASSERT(false, "Cannot allocate memory for device: " << dev.to_string() << "\nError: " << ex.what());
        }
    }

    // create ipc handle from memory handles
    output << "PID: " << pid <<  " create ipc handle from memory handles" << std::endl;
    using ipc_handle_container = std::vector<ccl_device::device_ipc_memory_handle>;
    std::map<ccl_device*, ipc_handle_container> device_ipc_mem_storage;
    for(auto &device_storage : device_allocated_memory_storage)
    {
        // get device with its memory handles
        ccl_device* device_ptr = device_storage.first;
        try
        {
            mem_handles_container& memory_handles = device_storage.second;

            // convert mem handles to ipc memory handles
            ipc_handle_container& out_ipc_handles = device_ipc_mem_storage[device_ptr];
            out_ipc_handles.reserve(memory_handles.size());
            std::transform(memory_handles.begin(), memory_handles.end(),
                           std::back_inserter(out_ipc_handles),
                           [device_ptr](ccl_device::device_memory<int>& memory_handle)
                           {
                                return device_ptr->create_ipc_memory_handle(memory_handle.handle);
                           });
        }
        catch(const std::exception& ex)
        {
            UT_ASSERT(false, "Cannot create ip memory handle for device: "
                             << device_ptr->to_string() << "\nError: " << ex.what());
        }
    }


    // serialize ipc memory handles
    output << "PID: " << pid <<  " serialize IPC handles" << std::endl;
    std::vector<uint8_t> serialized_raw_handles;
    for(auto& device_storage : device_ipc_mem_storage)
    {
        // get device with its ipc handles
        ipc_handle_container& ipc_mem_handles = device_storage.second;

        // serialzie handles
        size_t offset = 0;
        for(const auto& ipc_mem_h : ipc_mem_handles)
        {
            try
            {
                offset += ipc_mem_h.serialize(serialized_raw_handles, offset);
            }
            catch(const std::exception& ex)
            {
                UT_ASSERT(false, "Cannot serialize ip memory handle: " << to_string(ipc_mem_h.handle)
                                 << "\nError: " << ex.what());
            }
        }
    }

    // send to other process
    output << "PID: " << pid <<  " write to socket" << std::endl;
    int ret = writeToSocket(communication_socket, serialized_raw_handles.data(), serialized_raw_handles.size());
    UT_ASSERT(ret == 0, "Cannot writeToSocket: " << strerror(errno));

    // read from other process
    output << "PID: " << pid <<  " read from socket" << std::endl;
    std::vector<uint8_t> received_raw_handles(serialized_raw_handles.size(), 0); // size to receive must be equal with send
    readFromSocket(communication_socket, received_raw_handles.data(), received_raw_handles.size());
    UT_ASSERT(ret == 0, "Cannot readFromSocket" << strerror(errno));

    // deserialize handles
    using ipc_memory_container = std::vector<ccl_device::device_ipc_memory>;
    std::map<ccl_device*, ipc_memory_container> foreign_device_ipc_mem_storage;
    size_t recv_data_size = received_raw_handles.size();
    const uint8_t* recv_data_start = received_raw_handles.data();
    output << "PID: " << pid <<  " deserialize  IPC handles" << std::endl;
    while(recv_data_size > 0)
    {
        try
        {
            auto recv_ipc_handle =
                ccl_device::device_ipc_memory_handle::deserialize<ccl_device::device_ipc_memory_handle>(&recv_data_start,
                                                                                                        recv_data_size,
                                                                                                        *global_platform);

            std::shared_ptr<ccl_device> owner_device = recv_ipc_handle->get_owner().lock();
            auto ipc_device_it = std::find_if(global_driver.devices.begin(), global_driver.devices.end(),
                                              [owner_device](typename ccl_device_driver::devices_storage_type::value_type& dev)
                                              {
                                                  return dev.second->handle == owner_device->handle;
                                              });
            UT_ASSERT(ipc_device_it != global_driver.devices.end(), "Cannot find ipc device in global driver");

            ipc_memory_container& container = foreign_device_ipc_mem_storage[owner_device.get()];
            container.push_back(owner_device->get_ipc_memory(std::move(recv_ipc_handle)));
        }
        catch(const std::exception& ex)
        {
            std::stringstream ss;
            std::copy(recv_data_start, recv_data_start + recv_data_size,
                      std::ostream_iterator<int>(ss, ","));
            UT_ASSERT(false, "Cannot deserialize ipc memory handle at: " << ss.str()
                             << "\n. Offset: " <<  recv_data_start - received_raw_handles.data()
                             << ". Error: " << ex.what());
        }
    }

    UT_ASSERT(foreign_device_ipc_mem_storage.size() == device_ipc_mem_storage.size(),
              "Reseived IPC handles  from foreign process is not equal with expected count");
#if 0
//prepare kernels
    output << "PID: " << pid <<  " prepare kernels" << std::endl;
    ze_kernel_desc_t desc = { ZE_KERNEL_DESC_VERSION_CURRENT,
                              ZE_KERNEL_FLAG_NONE   };
    desc.pKernelName = "test_main";
    std::map<ccl_device*, ze_kernel_handle_t> device_kernels;
    for(auto &dev_it : device_modules)
    {
        ccl_device* dev = dev_it.first;
        ccl_device::device_module& module = dev_it.second;

        ze_kernel_handle_t handle = nullptr;
        try
        {
            ze_result_t result = zeKernelCreate(module.handle, &desc, &handle);
            if (result != ZE_RESULT_SUCCESS)
            {
                throw std::runtime_error(std::string("Cannot create kernel: ") +
                                         desc.pKernelName +
                                         ", error: " + native::to_string(result));
            }

            //bind arguments
            auto& mem_container = device_allocated_memory_storage[dev];
            auto& my_send_mem = mem_container.at(0);
            auto& my_recv_mem = mem_container.at(1);

            int index = 0;
            result = zeKernelSetArgumentValue(handle, index, sizeof(my_send_mem.handle), &my_send_mem.handle);
            if (result != ZE_RESULT_SUCCESS)
            {
                throw std::runtime_error(std::string("Cannot zeKernelSetArgumentValue at: ") +
                                        std::to_string(index) + " index\nError: " +
                                        native::to_string(result));
            }
            index++;

            result = zeKernelSetArgumentValue(handle, index, sizeof(my_recv_mem.handle), &my_recv_mem.handle);
            if (result != ZE_RESULT_SUCCESS)
            {
                throw std::runtime_error(std::string("Cannot zeKernelSetArgumentValue at: ") +
                                        std::to_string(index) + " index\nError: " +
                                        native::to_string(result));
            }
            index++;

            //TODO bind ipc memory from first device in ipc devices
            for(auto &ip_device_pair : foreign_device_ipc_mem_storage)
            {
                ccl_device* ipc_device = ip_device_pair.first;
                (void)ipc_device;
                ipc_memory_container& ipc_mem_cont = ip_device_pair.second;

                try
                {
                    for(auto &mem : ipc_mem_cont)
                    {
                        result = zeKernelSetArgumentValue(handle, index, sizeof(mem.handle), &mem.handle);
                        if (result != ZE_RESULT_SUCCESS)
                        {
                            throw std::runtime_error(std::string("Cannot zeKernelSetArgumentValue at: ") +
                                                std::to_string(index) + " index\nError: " +
                                                native::to_string(result));
                        }
                        index++;
                    }
                }
                catch (const std::exception& ex)
                {
                    throw std::runtime_error(std::string("Cannot bind IPC memory from device: ") +
                                             ipc_device->to_string() +
                                             ex.what());
                }

                //TODO Only one device at now
                break;
            }
            device_kernels.emplace(dev, std::move(handle));
        }
        catch(const std::exception& ex)
        {
            UT_ASSERT(false, "Device: " << dev->to_string()
                             << "\nError: "<< ex.what());

            //TODO destroy kernel
        }
    }
#endif
    output << "PID: " << pid <<  " finalize process" << std::endl;
    if(pid == 0)
    {
        quick_exit(0);
    }
}
