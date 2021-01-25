#pragma once

#include <memory>
#include <sstream>
#include <cstdint>

#include "bcast_fixture.hpp"

DEFINE_KERNEL_TYPES(bcast)

namespace ring_single_device_case {

namespace ring_bcast_case {}

TYPED_TEST_CASE(ring_bcast_single_device_fixture, TestTypes);
TYPED_TEST(ring_bcast_single_device_fixture, ring_bcast_single_device_mt) {
    using namespace native;

    // Type of our test
    using native_type = TypeParam;

    // test case data
    const size_t buffer_size = 512;
    const int num_thread = 5;
    constexpr size_t mem_group_count = 1; //3;
    constexpr size_t flag_group_count = 3;

    std::shared_ptr<ccl_context> ctx;

    handles_storage<native_type> memory_storage(mem_group_count * num_thread);
    handles_storage<int> flags_storage(flag_group_count * num_thread);
    std::map<int, std::vector<int>> comm_param_storage;

    // check global driver
    auto drv_it = this->local_platform->drivers.find(0);
    UT_ASSERT(drv_it != this->local_platform->drivers.end(), "Driver by idx 0 must exist!");
    ccl_device_driver& driver = *drv_it->second;

    // check devices per process
    UT_ASSERT(driver.devices.size() == this->local_affinity.size(),
              "Count: %" << driver.devices.size() << ", bits: " << this->local_affinity.size()
                         << "Device count is not equal to affinity mask!");

    // device memory stencil data
    std::vector<native_type> send_values(buffer_size);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<native_type> recv_values(buffer_size, 0);

    // allocate device memory
    auto dev_it = driver.devices.begin();
    ccl_device& device = *dev_it->second;
    int root = 2;

    for (int thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        try {
            //initialize communication params
            int rank_idx = thread_idx;
            int rank_size = num_thread;
            size_t elem_count = buffer_size;

            comm_param_storage[thread_idx].push_back(rank_idx);
            comm_param_storage[thread_idx].push_back(rank_size);
            comm_param_storage[thread_idx].push_back(elem_count);
            comm_param_storage[thread_idx].push_back(root);

            //allocate flags & memory
            // memory
            auto mem_buf = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);

            if (thread_idx == root) {
                mem_buf.enqueue_write_sync(send_values);
            }
            else {
                mem_buf.enqueue_write_sync(recv_values);
            }

            /* fill array in specific order
             * Left: l_send, l_recv, l_tmp_recv, r_tmp_recv
             * Right: r_send, r_recv, r_tmp_recv, l_tmp_recv
             */
            memory_storage.register_shared_data(thread_idx, num_thread, std::move(mem_buf));

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
            flags_storage.register_shared_data(thread_idx,
                                               num_thread,
                                               std::move(left_wrote_2_me_flag),
                                               std::move(read_for_receive_flag),
                                               std::move(barrier_flag));
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
    //Check memory handles

    //prepare kernels in multithreading environment
    ze_kernel_desc_t desc = {
        .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
        .pNext = nullptr,
        .flags = 0,
    };
    desc.pKernelName = bcast_param_traits<native_type>::kernel_name;
    this->output << "KERNEL_NAME: " << desc.pKernelName << std::endl;

    std::map<size_t, ze_kernel_handle_t> thread_kernels;
    std::map<size_t, ccl_device::device_queue> thread_queue;
    std::map<size_t, ccl_device::device_cmd_list> thread_cmd_list;
    ccl_device::device_module& module = *(this->device_modules.find(&device)->second);
    for (int thread_idx = 0; thread_idx < num_thread; thread_idx++) {
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
    auto& out = this->output;

    //Set args and launch kernel
    std::mutex thread_lock; //workaround
    std::atomic<size_t> val{ 0 }; //workaround
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    for (auto& idx_kernel : thread_kernels) {
        size_t thread_idx = idx_kernel.first;

        ze_kernel_handle_t kernel = idx_kernel.second;
        auto& mem_handles = find_storage_val(memory_storage.per_thread_storage, thread_idx);
        auto& flag_handles = find_storage_val(flags_storage.per_thread_storage, thread_idx);
        auto& comm_handles = find_storage_val(comm_param_storage, thread_idx);

        //WORKAROUND: ONLY ONE LIST & QUEUE!
        ccl_device::device_queue& queue = thread_queue.find(0)->second;
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
                                   &thread_lock,
                                   &val,
                                   raw_out]() {
            (void)driver;
            (void)device;
            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { 1, 1, 1 };
            try {
                out << "Binding kernels arguments for thread:" << thread_idx << std::endl;
                // bind rank, size, buffer_size
                out << "thread_idx: " << thread_idx << " - "
                    << "comm_offset" << std::endl;
                std::array<int, 4> comm_offset{ 0, 1, 2, 10 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                bind_kernel_args(kernel, thread_idx, comm_offset, comm_handles);

                // bind l_send, l_recv, l_tmp, , , r_tmp
                // bind l_send, l_recv, , , r_recv, ,
                out << "thread_idx: " << thread_idx << " - "
                    << "mem_offset" << std::endl;
                std::array<int, mem_group_count * 2> mem_offset{ 3, 7 };
                bind_kernel_args(kernel, thread_idx, mem_offset, mem_handles);

                // bind left_wrote_2_me_flag, read_for_receive_flag, local_barrier_flag
                out << "thread_idx: " << thread_idx << " - "
                    << "flag_offset" << std::endl;
                std::array<int, flag_group_count * 2> flag_offset{ 4, 5, 6, 8, 9, -1 };
                bind_kernel_args(kernel, thread_idx, flag_offset, flag_handles);

                ze_result_t ret = ZE_RESULT_SUCCESS;
                {
                    // lock before submitting to the command list
                    std::lock_guard<std::mutex> lock(thread_lock);

                    ret = zeCommandListAppendLaunchKernel(
                        list.handle, kernel, &launch_args, nullptr, 0, nullptr);
                    if (ret != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("cannot zeCommandListAppendLaunchKernel, error: ") +
                            std::to_string(ret));
                    }
                    val++;
                }

                // sync and make sure all threads have arrived up to this point.
                while (val < num_thread) {
                }

                // let thread 0 to be the one submitting commands to the queue and sync
                if (thread_idx == 0) {
                    queue_sync_processing(list, queue);
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

    size_t corr_val = 0;
    try {
        for (auto& idx_kernel : thread_kernels) {
            size_t thread_idx = idx_kernel.first;
            auto lambda = [&corr_val](const int root,
                                      size_t thread_idx,
                                      size_t buffer_size,
                                      native_type value) -> bool {
                corr_val++;
                if (corr_val > buffer_size)
                    corr_val = 1;
                if (value != static_cast<native_type>(corr_val))
                    return false;
                return true;
            };

            memory_storage.check_results(thread_idx, out, 0, lambda, root, thread_idx, buffer_size);
        }
    }
    catch (check_on_exception& ex) {
        out << "Check results: \n";
        //printout
        // out << "Send memory:" << std::endl;
        // memory_storage.dump_by_index(output, 0 /*send_mem*/);
        out << "\nRecv memory:" << std::endl;
        memory_storage.dump_by_index(out, 0 /*recv_mem*/);

        std::stringstream ss;
        ss << ex.what() << ", But expected: " << corr_val << std::endl;
        UT_ASSERT(false, ss.str());
    }
}

} // namespace ring_single_device_case
