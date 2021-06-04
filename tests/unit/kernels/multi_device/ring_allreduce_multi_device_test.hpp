#pragma once

#include "allreduce_fixture.hpp"

DEFINE_KERNEL_TYPES_FOR_OP(allreduce, sum);
DEFINE_KERNEL_TYPES_FOR_OP(allreduce, prod);
DEFINE_KERNEL_TYPES_FOR_OP(allreduce, min);
DEFINE_KERNEL_TYPES_FOR_OP(allreduce, max);

namespace ring_multi_device_case {

TYPED_TEST_CASE(ring_allreduce_single_process_fixture, TestTypesAndOps);
TYPED_TEST(ring_allreduce_single_process_fixture, ring_allreduce_multi_device_mt) {
    using namespace native;
    // test case data
    const size_t buffer_size = 512;
    size_t comm_size = 2;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;

    ze_device_mem_alloc_desc_t mem_descr{
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = 0,
        .ordinal = 0,
    };

    // check global driver
    auto drv_it = local_platform->drivers.find(0);
    UT_ASSERT(drv_it != local_platform->drivers.end(), "Driver by idx 0 must exist!");
    auto driver = drv_it->second;

    // device per thread
    comm_size = driver->devices.size();
    UT_ASSERT(
        driver->devices.size() > 1,
        "MultileDevice test scope require at least 2 devices in local platform! Use correct \""
            << ut_device_affinity_mask_name << "\"");

    handles_storage<native_type> memory_storage(42 * comm_size);
    handles_storage<int> flags_storage(42 * comm_size);
    std::map<int, std::vector<int>> comm_param_storage;

    std::shared_ptr<ccl_context> ctx;

    // device memory stencil data
    std::vector<native_type> send_values(buffer_size);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<native_type> recv_values(buffer_size, 0);

    int rank = 0;
    for (auto dev_it = driver->devices.begin(); dev_it != driver->devices.end(); ++dev_it) {
        auto& device = *dev_it->second;

        //initialize communication params
        this->output << "Create device memory & flags handles for device by index: "
                     << std::to_string(device.get_device_id()) << ", as rank: (" << rank << "/"
                     << comm_size << ")" << std::endl;

        comm_param_storage[rank].push_back(rank);
        comm_param_storage[rank].push_back(comm_size);
        comm_param_storage[rank].push_back(buffer_size);

        //allocate flags & memory
        // memory
        this->output << "Alloc memory handles: " << std::endl;
        auto mem_send = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
        auto mem_recv = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
        auto temp_recv =
            device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx, mem_descr);
        mem_send.enqueue_write_sync(send_values);
        mem_recv.enqueue_write_sync(recv_values);
        temp_recv.enqueue_write_sync(recv_values);

        /* fill array in specific order
         * Left: l_send, l_recv, l_tmp_recv, r_tmp_recv
         * Right: r_send, r_recv, r_tmp_recv, l_tmp_recv
         */
        memory_storage.register_shared_data(
            rank, comm_size, std::move(mem_send), std::move(mem_recv), std::move(temp_recv));

        // flags
        auto left_wrote_2_me_flag = device.alloc_memory<int>(1, sizeof(int), ctx, mem_descr);
        auto read_for_receive_flag = device.alloc_memory<int>(1, sizeof(int), ctx, mem_descr);
        auto barrier_flag = device.alloc_memory<int>(1, sizeof(int), ctx);
        left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
        read_for_receive_flag.enqueue_write_sync({ (int)0 });
        barrier_flag.enqueue_write_sync({ (int)0 });

        /* fill array in specific order
         * Left: l_L, l_R, l_B, r_L, r_R
         * Right: r_L, r_R, r_B, l_L, L_R
         */
        flags_storage.register_shared_data(rank,
                                           comm_size,
                                           std::move(left_wrote_2_me_flag),
                                           std::move(read_for_receive_flag),
                                           std::move(barrier_flag));
        rank++;
    }

    for (size_t rank = 0; rank < driver->devices.size(); rank++) {
        memory_storage.rotate_shared_data(rank, driver->devices.size(), mem_group_count);
        flags_storage.rotate_shared_data(rank, driver->devices.size(), flag_group_count);
    }

    //prepare kernels
    ze_kernel_desc_t desc = {
        .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
        .pNext = nullptr,
        .flags = 0,
        .pKernelName = "allreduce_execution_float",
    };

    std::map<size_t, ze_kernel_handle_t> rank_kernels;
    std::map<size_t, ccl_device::device_queue> rank_queues;
    std::map<size_t, ccl_device::device_cmd_list> rank_cmd_lists;

    rank = 0;
    for (auto dev_it = driver->devices.begin(); dev_it != driver->devices.end(); ++dev_it) {
        ccl_device& device = *dev_it->second;
        ccl_device::device_module& module = *(device_modules.find(&device)->second);

        this->output << "Preparing kernels params: name of kernel: " << desc.pKernelName << "\n"
                     << "  device id: " << ccl::to_string(device.get_device_path()) << "\n"
                     << "  rank: " << rank << std::endl;

        ze_kernel_handle_t handle = nullptr;
        try {
            ze_result_t result = zeKernelCreate(module.handle, &desc, &handle);
            if (result != ZE_RESULT_SUCCESS) {
                throw std::runtime_error(std::string("Cannot create kernel: ") + desc.pKernelName +
                                         ", error: " + native::to_string(result));
            }

            this->output << "Create list & queue with default properties on device by id: "
                         << ccl::to_string(device.get_device_path()) << std::endl;

            rank_kernels.emplace(rank, std::move(handle));
            rank_queues.emplace(rank, device.create_cmd_queue(ctx));
            rank_cmd_lists.emplace(rank, device.create_cmd_list(ctx));

            rank++;
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
    // using mem_handles_storage = typename handles_storage<native_type>::mem_handles_container;
    for (auto dev_it = driver->devices.begin(); dev_it != driver->devices.end(); ++dev_it) {
        ccl_device& device = *dev_it->second;
        size_t rank = thread_group.size();
        ze_kernel_handle_t kernel = rank_kernels.find(rank)->second;
        auto& mem_handles = find_storage_val(memory_storage.per_rank_storage, rank);
        auto& flag_handles = find_storage_val(flags_storage.per_rank_storage, rank);
        auto& comm_handles = find_storage_val(comm_param_storage, rank);

        this->output << "Launch kernel params: \n"
                     << " device id: " << ccl::to_string(device.get_device_path())
                     << ", rank: " << rank << std::endl;

        ccl_device::device_queue& queue = rank_queues.find(rank)->second;
        ccl_device::device_cmd_list& list = rank_cmd_lists.find(rank)->second;

        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();
        thread_group.emplace_back([this,
                                   driver,
                                   &device,
                                   rank,
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
                out << "rank: " << rank << ", comm_handles: \n";

                // bind rank, size, buffer_size
                int i = 0;
                std::array<int, 3> comm_offset{ 0, 1, 2 };
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
                std::array<int, 6> mem_offset{ 3, 4, 5, -1, -1, 9 };
                UT_ASSERT(mem_offset.size() == mem_handles.size(),
                          "mem_offset != mem_handles"
                              << "\nLog:\n"
                              << out.str());
                out << "rank: " << rank << ", mem_handles: \n";
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
                std::array<int, 6> flag_offset{ 6, 7, 8, 10, 11, -1 };
                UT_ASSERT(flag_offset.size() == flag_handles.size(),
                          "flag_offset != flag_handles"
                              << "\nLog:\n"
                              << out.str());
                out << "rank: " << rank << ", flag_handles: \n";
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
                UT_ASSERT(false, "Exception in rank: " << rank << "\nError: " << ex.what());
                throw;
            }
        });
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
