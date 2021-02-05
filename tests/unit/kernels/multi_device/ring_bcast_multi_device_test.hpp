#include "bcast_fixture.hpp"

// test case data
namespace ring_multi_device_case {

using native_type = float;
using ring_bcast_float_fixture = ring_bcast_single_process_fixture<native_type>;
TEST_F(ring_bcast_float_fixture, ring_bcast_multi_device_mt) {
    using namespace native;
    const size_t buffer_size = 512;
    int num_thread = 2;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;

    // check global driver
    auto drv_it = local_platform->drivers.find(0);
    UT_ASSERT(drv_it != local_platform->drivers.end(), "Driver by idx 0 must exist!");
    auto driver = drv_it->second;
    int root = 2;

    // device per thread
    num_thread = driver->devices.size();
    UT_ASSERT(
        driver->devices.size() > 1,
        "MultileDevice test scope require at least 2 devices in local platform! Use correct \""
            << ut_device_affinity_mask_name << "\"");

    handles_storage<native_type> memory_storage(42 * num_thread);
    handles_storage<int> flags_storage(42 * num_thread);
    std::map<int, std::vector<int>> comm_param_storage;

    std::shared_ptr<ccl_context> ctx;

    // device memory stencil data
    std::vector<native_type> send_values(buffer_size);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<native_type> recv_values(buffer_size, 0);

    int rank_device_idx = 0;
    for (auto dev_it = driver->devices.begin(); dev_it != driver->devices.end(); ++dev_it) {
        auto& device = *dev_it->second;

        //initialize communication params
        int rank_idx = rank_device_idx;
        int rank_size = driver->devices.size();
        size_t elem_count = buffer_size;

        this->output << "Create device memory & flags handles for device by index: "
                     << std::to_string(device.get_device_id()) << ", as rank: (" << rank_device_idx
                     << "/" << rank_size << ")" << std::endl;

        comm_param_storage[rank_device_idx].push_back(rank_idx);
        comm_param_storage[rank_device_idx].push_back(rank_size);
        comm_param_storage[rank_device_idx].push_back(elem_count);
        comm_param_storage[rank_device_idx].push_back(root);

        //allocate flags & memory
        // memory
        this->output << "Alloc memory handles: " << std::endl;
        auto mem_send = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
        auto mem_recv = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
        auto temp_recv = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
        if (rank_device_idx == root) {
            mem_send.enqueue_write_sync(send_values);
        }
        else {
            mem_send.enqueue_write_sync(recv_values);
        }
        mem_recv.enqueue_write_sync(recv_values);
        temp_recv.enqueue_write_sync(recv_values);

        /* fill array in specific order
         * Left: l_send, l_recv, l_tmp_recv, r_tmp_recv
         * Right: r_send, r_recv, r_tmp_recv, l_tmp_recv
         */
        memory_storage.register_shared_data(rank_device_idx,
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

        /* fill array in specific order
         * Left: l_L, l_R, l_B, r_L, r_R
         * Right: r_L, r_R, r_B, l_L, L_R
         */
        flags_storage.register_shared_data(rank_device_idx,
                                           num_thread,
                                           std::move(left_wrote_2_me_flag),
                                           std::move(read_for_receive_flag),
                                           std::move(barrier_flag));
        rank_device_idx++;
    }

    for (size_t rank_idx = 0; rank_idx < driver->devices.size(); rank_idx++) {
        memory_storage.rotate_shared_data(rank_idx, driver->devices.size(), mem_group_count);
        flags_storage.rotate_shared_data(rank_idx, driver->devices.size(), flag_group_count);
    }

    //prepare kernels
    ze_kernel_desc_t desc = {
        .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
        .pNext = nullptr,
        .flags = 0,
    };
    desc.pKernelName = "bcast_execution_float";
    std::map<size_t, ze_kernel_handle_t> thread_kernels;
    std::map<size_t, ccl_device::device_queue> thread_queue;
    std::map<size_t, ccl_device::device_cmd_list> thread_cmd_list;

    rank_device_idx = 0;
    for (auto dev_it = driver->devices.begin(); dev_it != driver->devices.end(); ++dev_it) {
        ccl_device& device = *dev_it->second;
        ccl_device::device_module& module = *(device_modules.find(&device)->second);

        this->output << "Preparing kernels params: name of kernel: " << desc.pKernelName << "\n"
                     << "  device id: " << ccl::to_string(device.get_device_path()) << "\n"
                     << "  Rank idx" << rank_device_idx << std::endl;

        ze_kernel_handle_t handle = nullptr;
        try {
            ze_result_t result = zeKernelCreate(module.handle, &desc, &handle);
            if (result != ZE_RESULT_SUCCESS) {
                throw std::runtime_error(std::string("Cannot create kernel: ") + desc.pKernelName +
                                         ", error: " + native::to_string(result));
            }

            this->output << "Create list & queue with default properties on device by id: "
                         << ccl::to_string(device.get_device_path()) << std::endl;

            thread_kernels.emplace(rank_device_idx, std::move(handle));
            thread_queue.emplace(rank_device_idx, device.create_cmd_queue(ctx));
            thread_cmd_list.emplace(rank_device_idx, device.create_cmd_list(ctx));

            rank_device_idx++;
        }
        catch (const std::exception& ex) {
            throw std::runtime_error(std::string("Error: ") + ex.what());
        }
    }

    //printout
    memory_storage.dump(output, true);

    //Set args and launch kernel
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    size_t thread_idx = 0;
    for (auto dev_it = driver->devices.begin(); dev_it != driver->devices.end(); ++dev_it) {
        ccl_device& device = *dev_it->second;
        ze_kernel_handle_t kernel = thread_kernels.find(thread_idx)->second;

        auto& mem_handles = find_storage_val(memory_storage.per_thread_storage, thread_idx);
        auto& flag_handles = find_storage_val(flags_storage.per_thread_storage, thread_idx);
        auto& comm_handles = find_storage_val(comm_param_storage, thread_idx);

        this->output << "Launch kernel params: \n"
                     << " Device idx" << ccl::to_string(device.get_device_path()) << ",  Rank idx"
                     << rank_device_idx << std::endl;

        ccl_device::device_queue& queue = thread_queue.find(thread_idx)->second;
        ccl_device::device_cmd_list& list = thread_cmd_list.find(thread_idx)->second;

        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();
        thread_group.emplace_back([this,
                                   driver,
                                   &device,
                                   thread_idx,
                                   kernel,
                                   &list,
                                   &queue,
                                   &mem_handles,
                                   &flag_handles,
                                   &comm_handles,
                                   raw_out]() {
            (void)driver;
            (void)device;

            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { 1, 1, 1 };
            try {
                ze_result_t result;
                out << "thread_idx: " << thread_idx << ", comm_handles: \n";

                // bind rank, size, buffer_size
                int i = 0;
                std::array<int, 4> comm_offset{ 0, 1, 2, 11 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(),
                          "comm_offset != comm_handles"
                              << "\nLog:\n"
                              << out.str());
                for (auto& comm : comm_handles) {
                    out << "index: " << comm_offset[i] << ": " << comm << std::endl;
                    result = zeKernelSetArgumentValue(kernel, comm_offset[i], sizeof(comm), &comm);
                    if (result != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("Cannot zeKernelSetArgumentValue memory at comm_offset: ") +
                            std::to_string(comm_offset[i]) +
                            " index\nError: " + native::to_string(result) + "\nLog:\n" + out.str());
                    }
                    i++;
                }
                out << std::endl;

                // bind l_send, l_recv, l_tmp, , , r_tmp
                i = 0;
                std::array<int, 6> mem_offset{ 3, 4, -1, -1, 8, -1 };
                UT_ASSERT(mem_offset.size() == mem_handles.size(),
                          "mem_offset != mem_handles"
                              << "\nLog:\n"
                              << out.str());
                out << "thread_idx: " << thread_idx << ", mem_handles: \n";
                for (auto& mem : mem_handles) {
                    if (mem_offset[i] == -1) {
                        i++;
                        continue; //skip this argument
                    }

                    out << "index: " << mem_offset[i] << ": " << mem << std::endl;
                    result = zeKernelSetArgumentValue(kernel, mem_offset[i], sizeof(mem), &mem);
                    if (result != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("Cannot zeKernelSetArgumentValue memory at mem_offset: ") +
                            std::to_string(mem_offset[i]) +
                            " index\nError: " + native::to_string(result) + "\nLog:\n" + out.str());
                    }
                    i++;
                }
                out << std::endl;

                // bindleft_wrote_2_me_flag, read_for_receive_flag, local_barrier_flag
                i = 0;
                std::array<int, 6> flag_offset{ 5, 6, 7, 9, 10, -1 };
                UT_ASSERT(flag_offset.size() == flag_handles.size(),
                          "flag_offset != flag_handles"
                              << "\nLog:\n"
                              << out.str());
                out << "thread_idx: " << thread_idx << ", flag_handles: \n";
                for (auto& flag : flag_handles) {
                    if (flag_offset[i] == -1) {
                        i++;
                        continue; //skip this argument
                    }
                    out << "index: " << flag_offset[i] << ": " << flag << std::endl;
                    result = zeKernelSetArgumentValue(kernel, flag_offset[i], sizeof(flag), &flag);
                    if (result != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("Cannot zeKernelSetArgumentValue flags at flag_offset: ") +
                            std::to_string(flag_offset[i]) +
                            " index\nError: " + native::to_string(result) + "\nLog:\n" + out.str());
                    }
                    i++;
                }
                out << std::endl;

                ze_result_t ret = zeCommandListAppendLaunchKernel(
                    list.handle, kernel, &launch_args, nullptr, 0, nullptr);
                if (ret != ZE_RESULT_SUCCESS) {
                    throw std::runtime_error(
                        std::string("cannot zeCommandListAppendLaunchKernel, error: ") +
                        std::to_string(ret) + "\nLog:\n" + out.str());
                }

                ret = zeCommandListClose(list.handle);
                if (ret != ZE_RESULT_SUCCESS) {
                    throw std::runtime_error(std::string("cannot zeCommandListClose, error: ") +
                                             std::to_string(ret) + "\nLog:\n" + out.str());
                }

                ret = zeCommandQueueExecuteCommandLists(queue.handle, 1, &list.handle, nullptr);
                if (ret != ZE_RESULT_SUCCESS) {
                    throw std::runtime_error(
                        std::string("cannot zeCommandQueueExecuteCommandLists, error: ") +
                        std::to_string(ret) + "\nLog:\n" + out.str());
                }

                ret = zeCommandQueueSynchronize(queue.handle, std::numeric_limits<uint32_t>::max());
                if (ret != ZE_RESULT_SUCCESS) {
                    throw std::runtime_error(
                        std::string("cannot zeCommandQueueSynchronize, error: ") +
                        std::to_string(ret) + "\nLog:\n" + out.str());
                }
                out << "thread finished" << std::endl;
            }
            catch (const std::exception& ex) {
                UT_ASSERT(false, "Exception in thread: " << thread_idx << "\nError: " << ex.what());
                throw;
            }
        });

        thread_idx++;
        thread_out_put.push_back(std::move(out_ptr));
    }

    size_t index = 0;
    for (auto& t : thread_group) {
        t.join();
        output << thread_out_put[index]->str() << std::endl;
        index++;
    }

    for (auto& t : thread_group) {
        t.join();
    }

    //printout
    memory_storage.dump(output);
}
} // namespace ring_multi_device_case
