#pragma once
#if 0
#include <memory>
#include <sstream>

#include "ipc_fixture.hpp"

namespace ring_single_device_case {

using native_type = float;

TEST_F(ring_ipc_allreduce_single_device_fixture, ring_ipc_allreduce_single_device) {
    using namespace native;

    // test case data
    const size_t buffer_size = 512;
    const size_t num_thread = 1;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;
    constexpr size_t ipc_mem_group_count = 1;
    constexpr size_t ipc_flag_group_count = 2;

    //TODO: ctx
    std::shared_ptr<ccl_context> ctx;

    handles_storage<native_type> memory_storage(42 * num_thread);
    handles_storage<int> flags_storage(42 * num_thread);
    ipc_server_handles_storage<native_type> ipc_server_memory(42 * num_thread);
    ipc_server_handles_storage<int> ipc_server_flags(42 * num_thread);

    ipc_client_handles_storage<native_type> ipc_client_memory;
    ipc_client_handles_storage<int> ipc_client_flags;

    std::map<size_t, std::vector<size_t>> comm_param_storage;

    // check global driver
    auto drv_it = local_platform->drivers.find(0);
    UT_ASSERT(drv_it != local_platform->drivers.end(), "Driver by idx 0 must exist!");
    ccl_device_driver& driver = *drv_it->second;

    // check devices per process
    UT_ASSERT(driver.devices.size() == local_affinity.size(),
              "Count: " << driver.devices.size() << ", bits: " << local_affinity.size()
                        << "Device count is not equal to affinity mask!");

    std::vector<size_t> thread_indices;

    // device memory stencil data
    std::vector<native_type> send_values(buffer_size);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<native_type> recv_values(buffer_size, 0);

    // allocate device memory
    auto dev_it = driver.devices.begin();
    ccl_device& device = *dev_it->second;

    for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        thread_indices.push_back(thread_idx);
        try {
            // initialize communication params
            size_t rank_idx = thread_idx + is_child();
            size_t rank_size = num_thread + 1; //forked
            size_t elem_count = buffer_size;

            comm_param_storage[thread_idx].push_back(rank_idx);
            comm_param_storage[thread_idx].push_back(rank_size);
            comm_param_storage[thread_idx].push_back(elem_count);

            // allocate flags & memory
            // memory
            auto mem_send = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
            auto mem_recv = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
            auto temp_recv =
                device.alloc_memory<native_type>(buffer_size / num_thread, sizeof(native_type), ctx);
            mem_send.enqueue_write_sync(send_values);
            mem_recv.enqueue_write_sync(recv_values);
            temp_recv.enqueue_write_sync(recv_values.begin(),
                                         recv_values.begin() + buffer_size / num_thread);

            ipc_server_memory.create_ipcs(thread_idx, num_thread, &temp_recv);

            /* fill array in specific order
             * l - left
             * r - right
             * Left: l_send, l_recv, l_tmp_recv, r_tmp_recv
             * Right: r_send, r_recv, r_tmp_recv, l_tmp_recv
             */
            memory_storage.register_shared_data(thread_idx,
                                                num_thread,
                                                std::move(mem_send),
                                                std::move(mem_recv),
                                                std::move(temp_recv));

            // flags
            auto left_wrote_2_me_flag = device.alloc_memory<int>(1, sizeof(int), ctx);
            auto read_for_receive_flag = device.alloc_memory<int>(1, sizeof(int), ctx);
            auto barrier_flag = device.alloc_memory<int>(1, sizeof(int), ctx);
            left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
            read_for_receive_flag.enqueue_write_sync({ (int)0 });
            barrier_flag.enqueue_write_sync({ (int)0 });

            ipc_server_flags.create_ipcs(
                thread_idx, num_thread, &left_wrote_2_me_flag, &read_for_receive_flag);

            /* fill array in specific order
             * Left: l_L, l_R, l_B, r_L, r_R
             * Right: r_L, r_R, r_B, l_L, L_R
             */
            flags_storage.register_shared_data(thread_idx,
                                               num_thread,
                                               std::move(left_wrote_2_me_flag),
                                               std::move(read_for_receive_flag),
                                               std::move(barrier_flag));
        }
        catch (const std::exception& ex) {
            UT_ASSERT(
                false,
                "Cannot allocate memory for thread: %" << thread_idx << "\nError: " << ex.what());
        }
    }

    for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        memory_storage.rotate_shared_data(thread_idx, num_thread, mem_group_count);
        flags_storage.rotate_shared_data(thread_idx, num_thread, flag_group_count);
    }

    //serialize
    std::vector<uint8_t> serialized_raw_handles;
    std::vector<std::vector<uint8_t>> memory_serialized;
    std::vector<std::vector<uint8_t>> flags_serialized;

    memory_serialized.resize(num_thread);
    flags_serialized.resize(num_thread);
    for (size_t thread_id = 0; thread_id < num_thread; thread_id++) {
        memory_serialized[thread_id] = ipc_server_memory.serialize_storage(thread_id);
        serialized_raw_handles.insert(serialized_raw_handles.end(),
                                      memory_serialized[thread_id].begin(),
                                      memory_serialized[thread_id].end());

        flags_serialized[thread_id] = ipc_server_flags.serialize_storage(thread_id);
        serialized_raw_handles.insert(serialized_raw_handles.end(),
                                      flags_serialized[thread_id].begin(),
                                      flags_serialized[thread_id].end());
    }

    // send to other process
    output << "PID: " << pid << " write to socket bytes: " << serialized_raw_handles.size()
           << std::endl;
    int ret = writeToSocket(
        communication_socket, serialized_raw_handles.data(), serialized_raw_handles.size());
    UT_ASSERT(ret == 0, "Cannot writeToSocket: " << strerror(errno));

    // read from other process
    std::vector<uint8_t> received_raw_handles(serialized_raw_handles.size(),
                                              0); // size to receive must be equal with send
    output << "PID: " << pid << " read from socket,expected bytes: " << received_raw_handles.size()
           << std::endl;
    readFromSocket(communication_socket, received_raw_handles.data(), received_raw_handles.size());
    UT_ASSERT(ret == 0, "Cannot readFromSocket" << strerror(errno));

    try {
        // deserialize
        for (size_t thread_id = 0; thread_id < num_thread; thread_id++) {
            //swap send - received arrays
            size_t mem_send = memory_serialized[thread_id].size();
            memory_serialized[thread_id].assign(received_raw_handles.begin(),
                                                received_raw_handles.begin() + mem_send);
            flags_serialized[thread_id].assign(received_raw_handles.begin() + mem_send,
                                               received_raw_handles.end());

            size_t count =
                ipc_client_memory.deserialize(memory_serialized.at(thread_id), 1, *global_platform);
            UT_ASSERT(count == 1, "Deserialized 1 IPC memory handle");
            count =
                ipc_client_flags.deserialize(flags_serialized.at(thread_id), 2, *global_platform);
            UT_ASSERT(count == 2, "Deserialized 2 IPC flag handles");
        }
    }
    catch (const std::exception& ex) {
        UT_ASSERT(false, "Cannot deserialize IPC data, error: " << ex.what());
    }

    //prepare kernels in multithreading environment
    ze_kernel_desc_t desc = {
        .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
        .pNext = nullptr,
        .flags = 0,
        .pKernelName = "allreduce_execution_float",
    };

    std::map<size_t, ze_kernel_handle_t> thread_kernels;
    std::map<size_t, ccl_device::device_queue> thread_queue;
    std::map<size_t, ccl_device::device_cmd_list> thread_cmd_list;
    ccl_device::device_module& module = *(device_modules.find(&device)->second);
    for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        //thread_group.emplace
        ze_kernel_handle_t handle = nullptr;
        try {
            ze_result_t result = zeKernelCreate(module.handle, &desc, &handle);
            if (result != ZE_RESULT_SUCCESS) {
                throw std::runtime_error(std::string("Cannot create kernel: ") + desc.pKernelName +
                                         ", error: " + native::to_string(result));
            }

            thread_kernels.emplace(thread_idx, std::move(handle));
            thread_queue.emplace(thread_idx, device.create_cmd_queue(ctx));
            thread_cmd_list.emplace(thread_idx, device.create_cmd_list(ctx));
        }
        catch (const std::exception& ex) {
            throw std::runtime_error(std::string("Error: ") + ex.what());
        }
    }

    //printout
    output << "PID: " << pid << "\n*************************\n";
    memory_storage.dump(output, true);
    output << std::endl;

    //Set args and launch kernel
    std::mutex thread_lock; //workaround
    std::atomic<size_t> val{ 0 }; //workaround
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    for (auto& idx_kernel : thread_kernels) {
        size_t thread_idx = idx_kernel.first;
        ze_kernel_handle_t kernel = idx_kernel.second;
        auto& mem_handles = memory_storage.per_thread_storage.find(thread_idx)->second;
        auto& flag_handles = flags_storage.per_thread_storage.find(thread_idx)->second;
        auto& comm_handles = comm_param_storage.find(thread_idx)->second;

        auto& ipc_mem_handles = ipc_client_memory.per_thread_storage.find(thread_idx)->second;
        auto& ipc_flag_handles = ipc_client_flags.per_thread_storage.find(thread_idx)->second;

        //WORKAROUND: ONLY ONE LIST & QUEUE!
        //ccl_device::device_queue& queue = thread_queue.find(thread_idx)->second;
        ccl_device::device_queue& queue = thread_queue.find(0)->second;
        //ccl_device::device_cmd_list& list = thread_cmd_list.find(thread_idx)->second;
        ccl_device::device_cmd_list& list = thread_cmd_list.find(0)->second;

        output << "PID: " << pid << ", start thread for kernel execution" << std::endl;
        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();
        thread_group.emplace_back([this,
                                   &driver,
                                   &device,
                                   thread_idx,
                                   kernel,
                                   &list,
                                   &queue,
                                   &mem_handles,
                                   &flag_handles,
                                   ipc_mem_handles,
                                   ipc_flag_handles,
                                   &comm_handles,
                                   &thread_lock,
                                   &val,
                                   raw_out]() {
            (void)driver;
            (void)device;
            (void)flag_handles;
            (void)thread_lock;
            (void)val;

            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { 1, 1, 1 };
            try {
                ze_result_t result;

                // bind rank, size, buffer_size
                out << "PID: " << pid << ", thread_idx: " << thread_idx << ", comm_handles: \n";
                size_t i = 0;
                std::array<int, 3> comm_offset{ 0, 1, 2 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                for (auto& comm : comm_handles) {
                    out << "index: " << comm_offset[i] << ": " << comm << std::endl;
                    result = zeKernelSetArgumentValue(kernel, comm_offset[i], sizeof(comm), &comm);
                    if (result != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("Cannot zeKernelSetArgumentValue memory at comm_offset: ") +
                            std::to_string(comm_offset[i]) +
                            " index\nError: " + native::to_string(result));
                    }
                    i++;
                }
                out << std::endl;

                // bind l_send, l_recv, l_tmp,
                i = 0;
                std::array<int, mem_group_count> mem_offset{ 3, 4, 5 };
                out << "PID: " << pid << ", thread_idx: " << thread_idx << ", mem_handles: \n";
                for (auto& mem : mem_handles) {
                    if (i >= mem_group_count) {
                        break;
                    }

                    int arg_offset = mem_offset.at(i);
                    out << "index: " << arg_offset << ": " << mem << std::endl;
                    result = zeKernelSetArgumentValue(kernel, arg_offset, sizeof(mem), &mem);
                    if (result != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("Cannot zeKernelSetArgumentValue memory at mem_offset: ") +
                            std::to_string(arg_offset) +
                            " index\nError: " + native::to_string(result));
                    }
                    i++;
                }
                out << std::endl;

                // Bind IPC memory
                try {
                    size_t i = 0;
                    std::array<int, ipc_mem_group_count> mem_offset{ 9 };
                    out << "PID: " << pid << ", thread_idx: " << thread_idx
                        << ", ipc_mem_handles: \n";
                    for (auto& mem : ipc_mem_handles) {
                        if (i >= ipc_mem_group_count) {
                            break;
                        }

                        int arg_offset = mem_offset.at(i);
                        out << "index: " << arg_offset << ": " << mem->handle.pointer << std::endl;
                        result = zeKernelSetArgumentValue(
                            kernel, arg_offset, sizeof(mem->handle), &mem->handle);
                        if (result != ZE_RESULT_SUCCESS) {
                            throw std::runtime_error(
                                std::string(
                                    "Cannot zeKernelSetArgumentValue memory at mem_offset: ") +
                                std::to_string(arg_offset) +
                                " index\nError: " + native::to_string(result));
                        }
                        i++;
                    }
                    out << std::endl;
                }
                catch (const std::exception& ex) {
                    out << "PID: " << pid << ", Cannot bind IPC memory, error: " << ex.what()
                        << std::endl;
                    throw;
                }

                // bindleft_wrote_2_me_flag, read_for_receive_flag, local_barrier_flag
                i = 0;
                std::array<int, flag_group_count> flag_offset{ 6, 7, 8 };
                out << "PID: " << pid << ", thread_idx: " << thread_idx << ", flag_handles: \n"
                    << std::endl;
                for (auto& flag : flag_handles) {
                    if (i >= flag_group_count) {
                        break;
                    }

                    out << "index: " << flag_offset[i] << ": " << flag << std::endl;
                    result = zeKernelSetArgumentValue(kernel, flag_offset[i], sizeof(flag), &flag);
                    if (result != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("Cannot zeKernelSetArgumentValue flags at flag_offset: ") +
                            std::to_string(flag_offset[i]) +
                            " index\nError: " + native::to_string(result));
                    }
                    i++;
                }

                // Bind IPC flags
                try {
                    size_t i = 0;
                    std::array<int, ipc_flag_group_count> mem_offset{ 10, 11 };
                    out << "PID: " << pid << ", thread_idx: " << thread_idx
                        << ", ipc_flag_handles: \n"
                        << std::endl;
                    for (auto& flag : ipc_flag_handles) {
                        int arg_offset = mem_offset.at(i);
                        out << "index: " << arg_offset << ": " << flag->handle.pointer << std::endl;
                        result = zeKernelSetArgumentValue(
                            kernel, arg_offset, sizeof(flag->handle), &flag->handle);
                        if (result != ZE_RESULT_SUCCESS) {
                            throw std::runtime_error(
                                std::string(
                                    "Cannot zeKernelSetArgumentValue flag at mem_offset: ") +
                                std::to_string(arg_offset) +
                                " index\nError: " + native::to_string(result));
                        }
                        i++;
                    }
                }
                catch (const std::exception& ex) {
                    out << "PID: " << pid << ", Cannot bind IPC flag, error: " << ex.what()
                        << std::endl;
                    throw;
                }

                // single process case: lock before submitting to the command list
                //thread_lock.lock();

                ze_result_t ret = zeCommandListAppendLaunchKernel(
                    list.handle, kernel, &launch_args, nullptr, 0, nullptr);
                if (ret != ZE_RESULT_SUCCESS) {
                    throw std::runtime_error(
                        std::string("cannot zeCommandListAppendLaunchKernel, error: ") +
                        std::to_string(ret));
                }
                //val++;

                //thread_lock.unlock();

                // sync and make sure all threads have arrived up to this point.
                /*while(val < num_thread)
                {
                }*/

                //sync processes
                uint8_t data = is_child(), ret_data = is_child();
                {
                    writeToSocket(communication_socket, &data, sizeof(data));
                    readFromSocket(communication_socket, &ret_data, sizeof(ret_data));
                    UT_ASSERT(data != ret_data, "invalid sync");
                }

                if (thread_idx == 0 /* && ret_data == 1*/ /*uncommet for master pid*/) {
                    std::cerr << "PID: " << pid << ", process close queue" << std::endl;

                    ret = zeCommandListClose(list.handle);
                    if (ret != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(std::string("cannot zeCommandListClose, error: ") +
                                                 std::to_string(ret));
                    }

                    ret = zeCommandQueueExecuteCommandLists(queue.handle, 1, &list.handle, nullptr);
                    if (ret != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("cannot zeCommandQueueExecuteCommandLists, error: ") +
                            std::to_string(ret));
                    }

                    do {
                        std::cerr << "PID: " << pid << ", start synchronize" << std::endl;
                        ret = zeCommandQueueSynchronize(queue.handle,
                                                        std::numeric_limits<uint32_t>::max());
                        if (ret != ZE_RESULT_SUCCESS && ret != ZE_RESULT_NOT_READY) {
                            throw std::runtime_error(
                                std::string("cannot zeCommandQueueSynchronize, error: ") +
                                std::to_string(ret));
                        }
                        out << "PID: " << pid << native::to_string(ret) << std::endl;
                        std::cerr << "PID: " << pid << native::to_string(ret) << std::endl;
                    } while (ret == ZE_RESULT_NOT_READY);
                    out << "PID: " << pid << ", thread finished" << std::endl;
                }
            }
            catch (const std::exception& ex) {
                UT_ASSERT(false,
                          "Exception in PID: " << pid << ",thread: " << thread_idx
                                               << "\nError: " << ex.what() << ", at phase:\n{\n"
                                               << out.str() << "\n}\n");
                throw;
            }
        });

        thread_out_put.push_back(std::move(out_ptr));
    }

    size_t index = 0;
    output << "PID: " << pid << ", wating threads" << std::endl;
    for (auto& t : thread_group) {
        //sleep(15);
        t.join();
        output << "PID: " << pid << "\n*************************\n"
               << thread_out_put[index]->str() << "\n*************************\n"
               << std::endl;

        //t.join();
        index++;
    }

    uint8_t ready = 0;
    if (is_child()) {
        readFromSocket(communication_socket, &ready, sizeof(ready));
    }
    else {
        ready = 1;
        writeToSocket(communication_socket, &ready, sizeof(ready));
    }
    output << "PID: " << pid << ", finished, status: " << ready << std::endl;
    //printout
    /*
    memory_storage.dump(output);
    flags_storage.dump(output);
    * */
}

} // namespace ring_single_device_case
#endif