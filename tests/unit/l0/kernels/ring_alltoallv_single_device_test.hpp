#pragma once

#include <memory>
#include <sstream>
#include "alltoallv_fixture.hpp"

DEFINE_KERNEL_TYPES(alltoallv)

namespace ring_single_device_case {

namespace ring_alltoallv_case {}

TYPED_TEST_CASE(ring_alltoallv_single_device_fixture, TestTypes);

TYPED_TEST(ring_alltoallv_single_device_fixture, ring_alltoallv_single_device_mt) {
    using namespace native;

    // Type of our test
    using native_type = TypeParam;

    // test case data
    const size_t num_thread = 4;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;

    const size_t send_buffer_base_size = 128;
    size_t total_recv_size = send_buffer_base_size * (num_thread / 2) * (1 + num_thread);
    std::vector<size_t> total_send_sizes(num_thread);
    for (size_t i = 0; i < num_thread; ++i) {
        total_send_sizes[i] = num_thread * send_buffer_base_size * (i + 1);
    }
    size_t tmp_buffer_size = send_buffer_base_size * num_thread * (num_thread - 1);

    std::map<int, std::vector<int>> comm_param_storage;
    std::map<size_t, std::vector<ccl_device::device_memory<size_t>>> comm_param_mem_storage;
    handles_storage<native_type> memory_storage(42 * num_thread);
    handles_storage<int> flags_storage(42 * num_thread);

    std::shared_ptr<ccl_context> ctx;

    // check global driver
    auto drv_it = this->local_platform->drivers.find(0);
    UT_ASSERT(drv_it != this->local_platform->drivers.end(), "Driver by idx 0 must exist!");
    ccl_device_driver& driver = *drv_it->second;

    // check devices per process
    UT_ASSERT(driver.devices.size() == this->local_affinity.size(),
              "Count: %" << driver.devices.size() << ", bits: " << this->local_affinity.size()
                         << "Device count is not equal to affinity mask!");

    std::vector<size_t> thread_indices;

    // device memory stencil data
    // send
    std::vector<std::vector<native_type>> send_values(num_thread);
    std::vector<std::vector<size_t>> send_counts(num_thread);
    std::vector<std::vector<size_t>> send_offsets(num_thread);

    size_t iota_base_value = 0;
    for (size_t idx = 0; idx < num_thread; idx++) {
        send_values[idx].resize(total_send_sizes[idx]);

        send_counts[idx].resize(num_thread, 0);
        send_offsets[idx].resize(num_thread, 0);
        iota_base_value += idx;
        for (size_t iter = 0; iter < send_counts[idx].size(); iter++) {
            send_counts[idx][iter] = send_buffer_base_size * (idx + 1);
            if (iter > 0)
                send_offsets[idx][iter] += send_offsets[idx][iter - 1] + send_counts[idx][iter - 1];

            std::iota(send_values[idx].begin() + send_offsets[idx][iter],
                      send_values[idx].begin() + send_offsets[idx][iter] + send_counts[idx][iter],
                      send_buffer_base_size * iota_base_value);
        }
    }

    // recv
    std::vector<std::vector<native_type>> recv_values(num_thread);
    std::vector<std::vector<size_t>> recv_counts(num_thread);
    std::vector<std::vector<size_t>> recv_offsets(num_thread);
    for (size_t idx = 0; idx < num_thread; idx++) {
        recv_values[idx].resize(total_recv_size, 0);
        recv_counts[idx].resize(num_thread, 0);
        recv_offsets[idx].resize(num_thread, 0);
        for (size_t iter = 0; iter < recv_counts[idx].size(); iter++) {
            recv_counts[idx][iter] = send_buffer_base_size * (iter + 1);
            if (iter > 0)
                recv_offsets[idx][iter] += recv_offsets[idx][iter - 1] + recv_counts[idx][iter - 1];
        }
    }

    // tmp
    std::vector<native_type> tmp_values(tmp_buffer_size, 0);

    // allocate device memory
    auto dev_it = driver.devices.begin();
    ccl_device& device = *dev_it->second;

    for (int thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        thread_indices.push_back(thread_idx);
        try {
            // initialize communication params
            int rank_idx = thread_idx;
            int rank_size = num_thread;
            // size_t send_count = recv_counts[thread_idx];

            comm_param_storage[thread_idx].push_back(rank_idx);
            comm_param_storage[thread_idx].push_back(rank_size);

            // send
            auto mem_send_counts = device.alloc_memory<size_t>(num_thread, sizeof(size_t), ctx);
            auto mem_send_offsets = device.alloc_memory<size_t>(num_thread, sizeof(size_t), ctx);

            mem_send_counts.enqueue_write_sync(send_counts[thread_idx]);
            mem_send_offsets.enqueue_write_sync(send_offsets[thread_idx]);

            comm_param_mem_storage[thread_idx].emplace_back(std::move(mem_send_counts));
            comm_param_mem_storage[thread_idx].emplace_back(std::move(mem_send_offsets));

            // recv
            auto mem_recv_counts = device.alloc_memory<size_t>(num_thread, sizeof(size_t), ctx);
            auto mem_recv_offsets = device.alloc_memory<size_t>(num_thread, sizeof(size_t), ctx);

            mem_recv_counts.enqueue_write_sync(recv_counts[thread_idx]);
            mem_recv_offsets.enqueue_write_sync(recv_offsets[thread_idx]);

            comm_param_mem_storage[thread_idx].emplace_back(std::move(mem_recv_counts));
            comm_param_mem_storage[thread_idx].emplace_back(std::move(mem_recv_offsets));

            // allocate flags & memory
            // memory
            auto mem_send = device.alloc_memory<native_type>(
                total_send_sizes[thread_idx], sizeof(native_type), ctx);
            auto mem_recv =
                device.alloc_memory<native_type>(total_recv_size, sizeof(native_type), ctx);
            auto mem_tmp =
                device.alloc_memory<native_type>(tmp_buffer_size, sizeof(native_type), ctx);

            mem_send.enqueue_write_sync(send_values[thread_idx]);
            mem_recv.enqueue_write_sync(recv_values[thread_idx]);
            mem_tmp.enqueue_write_sync(tmp_values);

            memory_storage.register_shared_data(thread_idx,
                                                num_thread,
                                                std::move(mem_send),
                                                std::move(mem_recv),
                                                std::move(mem_tmp));

            // flags
            auto left_wrote_2_me_flag = device.alloc_memory<int>(1, sizeof(int), ctx);
            auto ready_for_receive_flag = device.alloc_memory<int>(1, sizeof(int), ctx);
            auto proxy_size = device.alloc_memory<int>(1, sizeof(int), ctx);
            left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
            ready_for_receive_flag.enqueue_write_sync({ (int)0 });
            proxy_size.enqueue_write_sync({ (int)0 });

            /* fill array in specific order
             * Left: l_L, l_R, r_L, r_R
             * Right: r_L, r_R, l_L, L_R
             */
            flags_storage.register_shared_data(thread_idx,
                                               num_thread,
                                               std::move(left_wrote_2_me_flag),
                                               std::move(ready_for_receive_flag),
                                               std::move(proxy_size));
        }
        catch (const std::exception& ex) {
            UT_ASSERT(
                false,
                "Cannot allocate memory for thread: " << thread_idx << "\nError: " << ex.what());
        }
    }

    for (int thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        memory_storage.rotate_shared_data(thread_idx, num_thread, mem_group_count);
        flags_storage.rotate_shared_data(thread_idx, num_thread, flag_group_count);
    }

    // prepare kernels in multithreading environment
    ze_kernel_desc_t desc = {
        .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
        .pNext = nullptr,
        .flags = 0,
    };
    desc.pKernelName = alltoallv_param_traits<native_type>::kernel_name;
    std::map<size_t, ze_kernel_handle_t> thread_kernels;
    std::map<size_t, ccl_device::device_queue> thread_queue;
    std::map<size_t, ccl_device::device_cmd_list> thread_cmd_list;
    ccl_device::device_module& module = *(this->device_modules.find(&device)->second);
    for (int thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        // thread_group.emplace
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
    auto& out = this->output;
    // out << "L0 memory handles: " << std::endl;
    // memory_storage.dump(out, true);

    //Set args and launch kernel
    std::mutex thread_lock; //workaround
    std::atomic<size_t> val{ 0 }; //workaround
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    for (auto& idx_kernel : thread_kernels) {
        size_t thread_idx = idx_kernel.first;
        ze_kernel_handle_t kernel = idx_kernel.second;

        auto& comm_mem_handles = find_storage_val(comm_param_mem_storage, thread_idx);
        auto& mem_handles = find_storage_val(memory_storage.per_thread_storage, thread_idx);
        auto& flag_handles = find_storage_val(flags_storage.per_thread_storage, thread_idx);
        auto& comm_handles = find_storage_val(comm_param_storage, thread_idx);

        //WORKAROUND: ONLY ONE LIST & QUEUE!
        //ccl_device::device_queue& queue = thread_queue.find(thread_idx)->second;
        ccl_device::device_queue& queue = thread_queue.find(0)->second;
        //ccl_device::device_cmd_list& list = thread_cmd_list.find(thread_idx)->second;
        ccl_device::device_cmd_list& list = thread_cmd_list.find(0)->second;

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
                                   &comm_handles,
                                   &comm_mem_handles,
                                   &thread_lock,
                                   &val,
                                   raw_out]() {
            (void)driver;
            (void)device;
            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { 1, 1, 1 };
            try {
                ze_result_t result;
                out << "thread_idx: " << thread_idx << ", comm_handles: \n";

                // bind rank, size
                size_t i = 0;
                std::array<int, 2> comm_offset{ 0, 1 };
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

                // bind recv_counts, recv_offets
                i = 0;
                std::array<int, 4> comm_mem_offset{ 2, 3, 4, 5 };
                UT_ASSERT(comm_mem_offset.size() == comm_mem_handles.size(),
                          "comm_mem_offset != comm_mem_handles");
                for (auto& comm : comm_mem_handles) {
                    result =
                        zeKernelSetArgumentValue(kernel, comm_mem_offset[i], sizeof(comm), &comm);
                    if (result != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string(
                                "Cannot zeKernelSetArgumentValue memory at comm_mem_offset: ") +
                            std::to_string(comm_mem_offset[i]) +
                            " index\nError: " + native::to_string(result));
                    }

                    i++;
                }
                out << std::endl;

                // bind l_send, l_recv, r_recv
                i = 0;
                std::array<int, mem_group_count * 2> mem_offset{ 6, 7, 8, -1, -1, 9 };
                //UT_ASSERT(mem_offset.size() == mem_handles.size(), "mem_offset != mem_handles");
                out << "thread_idx: " << thread_idx << ", mem_handles: \n";
                for (auto& mem : mem_handles) {
                    if (i >= mem_group_count * 2) {
                        break; //only own+right is needed
                    }
                    if (mem_offset[i] == -1) {
                        i++;
                        continue; //skip this argument
                    }

                    out << "index: " << mem_offset[i] << ": " << (void*)mem << std::endl;
                    result = zeKernelSetArgumentValue(kernel, mem_offset[i], sizeof(mem), &mem);
                    if (result != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("Cannot zeKernelSetArgumentValue memory at mem_offset: ") +
                            std::to_string(mem_offset[i]) +
                            " index\nError: " + native::to_string(result));
                    }

                    i++;
                }
                out << std::endl;

                // bind left_wrote_2_me_flag, ready_for_receive_flag
                i = 0;
                std::array<int, flag_group_count * 2> flag_offset{ 10, 11, 12, 13, 14, 15 };
                //UT_ASSERT(flag_offset.size() == flag_handles.size(), "flag_offset != flag_handles");
                out << "thread_idx: " << thread_idx << ", flag_handles: \n";
                for (auto& flag : flag_handles) {
                    if (i >= flag_group_count * 2) {
                        break; //only own+right is needed
                    }

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
                            " index\nError: " + native::to_string(result));
                    }

                    i++;
                }
                out << std::endl;

                // lock before submitting to the command list
                thread_lock.lock();

                ze_result_t ret = zeCommandListAppendLaunchKernel(
                    list.handle, kernel, &launch_args, nullptr, 0, nullptr);
                if (ret != ZE_RESULT_SUCCESS) {
                    throw std::runtime_error(
                        std::string("cannot zeCommandListAppendLaunchKernel, error: ") +
                        std::to_string(ret));
                }
                val++;

                thread_lock.unlock();

                // sync and make sure all threads have arrived up to this point.
                while (val < num_thread) {
                }

                // let thread 0 to be the one submitting commands to the queue and sync
                if (thread_idx == 0) {
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

                    ret = zeCommandQueueSynchronize(queue.handle,
                                                    std::numeric_limits<uint32_t>::max());
                    if (ret != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("cannot zeCommandQueueSynchronize, error: ") +
                            std::to_string(ret));
                    }

                    out << "thread finished" << std::endl;
                }
            }
            catch (const std::exception& ex) {
                UT_ASSERT(false,
                          "Exception in thread: " << thread_idx << "\nError: " << ex.what()
                                                  << ", at pahse: " << out.str());
                throw;
            }
        });

        thread_out_put.push_back(std::move(out_ptr));
    }

    size_t index = 0;
    for (auto& t : thread_group) {
        t.join();
        out << "Kernels argument binding log for Thread: " << index << std::endl;
        out << thread_out_put[index]->str() << std::endl;
        index++;
    }

    size_t corr_val;
    try {
        for (auto& idx_kernel : thread_kernels) {
            size_t thread_idx = idx_kernel.first;
            corr_val = 0;

            auto lambda = [&corr_val](
                              size_t thread_idx, size_t num_thread, native_type value) -> bool {
                if (static_cast<native_type>(corr_val) != value)
                    return false;
                corr_val++;
                return true;
            };
            memory_storage.check_results(
                thread_idx, out, 1 /*recv_mem*/, lambda, thread_idx, num_thread);
        }
    }
    catch (check_on_exception& ex) {
        out << "Check results: \n";
        //printout
        out << "Send memory:" << std::endl;
        memory_storage.dump_by_index(out, 0 /*send_mem*/);
        out << "\nRecv memory:" << std::endl;
        memory_storage.dump_by_index(out, 1 /*recv_mem*/);

        std::stringstream ss;
        ss << ex.what() << ", But expected: " << corr_val << std::endl;
        UT_ASSERT(false, ss.str());
    }
}

} // namespace ring_single_device_case