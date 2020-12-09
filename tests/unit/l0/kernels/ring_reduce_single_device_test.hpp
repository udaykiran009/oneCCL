#pragma once

#include <memory>
#include <sstream>

#include "reduce_fixture.hpp"

DEFINE_KERNEL_TYPES_FOR_OP_BF16(reduce, add);
DEFINE_KERNEL_TYPES_FOR_OP_BF16(reduce, mult);
DEFINE_KERNEL_TYPES_FOR_OP_BF16(reduce, min);
DEFINE_KERNEL_TYPES_FOR_OP_BF16(reduce, max);

namespace ring_single_device_case {

namespace ring_reduce_case {}

TYPED_TEST_CASE(ring_reduce_single_device_fixture, TestTypesAndOpsReduction);

TYPED_TEST(ring_reduce_single_device_fixture, ring_reduce_single_device_mt) {
    using namespace native;

    // Type of our test
    using native_type = typename TypeParam::first_type;
    using op_type = typename TypeParam::second_type;

    std::shared_ptr<ccl_context> ctx;

    // test case data
    const size_t buffer_size = 512;
    const int num_thread = 4;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;

    handles_storage<native_type> memory_storage(mem_group_count * num_thread);
    handles_storage<int> flags_storage(flag_group_count * num_thread);
    std::map<int, std::vector<int>> comm_param_storage;
    handles_storage<native_type> host_results_storage(mem_group_count * num_thread);

    // check global driver
    auto drv_it = this->local_platform->drivers.find(0);
    UT_ASSERT(drv_it != this->local_platform->drivers.end(), "Driver by idx 0 must exist!");
    ccl_device_driver& driver = *drv_it->second;

    // check devices per process
    UT_ASSERT(driver.devices.size() == this->local_affinity.size(),
              "Count: %" << driver.devices.size() << ", bits: " << this->local_affinity.size()
                         << "Device count is not equal to affinity mask!");

    // device memory stencil data
    // Fill the data in the following order:
    // 0: 1 4 6 ...
    // 1: 2 3 5 ...
    // i.e. on each iteration different thread has min and max value
    // This allows to better test min/max ops.
    std::map<size_t, std::vector<native_type>> send_values;
    std::map<size_t, std::vector<native_type>> recv_values;
    std::map<size_t, std::vector<native_type>> calc_values;
    for (int thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        size_t mult = 0;
        for (size_t idx = 1; idx <= buffer_size; ++idx, ++mult) {
            send_values[thread_idx].push_back(
                static_cast<native_type>(idx * ((thread_idx + mult) % num_thread + 1)));
            recv_values[thread_idx].push_back(static_cast<native_type>(0));
            calc_values[thread_idx].push_back(static_cast<native_type>(0));
        }
    }
    //std::iota(send_values.begin(), send_values.end(), 1);
    //std::vector<native_type> recv_values(buffer_size, 0);

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
            auto mem_send = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
            auto mem_recv = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
            auto temp_recv = device.alloc_memory<native_type>(
                buffer_size / num_thread, sizeof(native_type), ctx);
            mem_send.enqueue_write_sync(send_values[thread_idx]);
            mem_recv.enqueue_write_sync(recv_values[thread_idx]);
            temp_recv.enqueue_write_sync(
                recv_values[thread_idx].begin(),
                recv_values[thread_idx].begin() + buffer_size / num_thread);

            /* fill array in specific order
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

    //prepare kernels in multithreading environment
    ze_kernel_desc_t desc = {
        .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
        .pNext = nullptr,
        .flags = 0,
    };
    desc.pKernelName = reduce_param_traits<native_type, op_type>::kernel_name;
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
                std::array<int, 4> comm_offset{ 0, 1, 2, 12 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                bind_kernel_args(kernel, thread_idx, comm_offset, comm_handles);

                // bind l_send, l_recv, l_tmp, , , r_tmp
                out << "thread_idx: " << thread_idx << " - "
                    << "mem_offset" << std::endl;
                std::array<int, mem_group_count * 2> mem_offset{ 3, 4, 5, -1, -1, 9 };
                bind_kernel_args(kernel, thread_idx, mem_offset, mem_handles);

                // bind left_wrote_2_me_flag, read_for_receive_flag, local_barrier_flag
                out << "thread_idx: " << thread_idx << " - "
                    << "flag_offset" << std::endl;
                std::array<int, flag_group_count * 2> flag_offset{ 6, 7, 8, 10, 11, -1 };
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

    // Copy device buffers back to host...
    for (auto& idx_kernel : thread_kernels) {
        size_t thread_idx = idx_kernel.first;
        auto& mem_handles = memory_storage.per_thread_storage.find(thread_idx)->second;
        size_t mem_idx = 1; // Recv memory
        size_t i = 0;
        for (auto iter_mem = mem_handles.begin(); iter_mem != mem_handles.end(); iter_mem++, i++) {
            if (i != mem_idx) {
                continue;
            }
            const auto* data = *iter_mem;
            auto it =
                std::find_if(memory_storage.allocated_storage.begin(),
                             memory_storage.allocated_storage.end(),
                             [data](const native::ccl_device::device_memory<native_type>& wrapper) {
                                 return wrapper.handle == data;
                             });

            recv_values[thread_idx] = it->enqueue_read_sync();
        }
    }

    // Compute output buffers in the same order as within the OCl kernel i.e. farthest rank -> right_rank -> right_right_rank -> ... -> root_rank ...
    size_t thread_idx_offset = (root + 1) % num_thread;
    size_t thread_idx;
    constexpr auto op = op_type{};
    for (size_t buffer_idx = 0; buffer_idx < buffer_size; ++buffer_idx) {
        native_type total_val = op.init();
        for (size_t thread_ctr = 0; thread_ctr < num_thread; ++thread_ctr) {
            thread_idx = (thread_ctr + thread_idx_offset) % num_thread;
            native_type temp_send_values = send_values[thread_idx][buffer_idx];
            total_val = op(total_val, temp_send_values);
        }
        calc_values[root][buffer_idx] = total_val;
    }

    // Check results against host...
    try {
        for (size_t thread_ctr = 0; thread_ctr < num_thread; ++thread_ctr) {
            for (size_t buffer_idx = 0; buffer_idx < buffer_size; ++buffer_idx) {
                if (recv_values[thread_idx][buffer_idx] != calc_values[thread_idx][buffer_idx]) {
                    throw comparison_exception(to_string(thread_idx),
                                               to_string(buffer_idx),
                                               to_string(recv_values[thread_idx][buffer_idx]),
                                               to_string(calc_values[thread_idx][buffer_idx]));
                }
            }
        }
    }
    catch (comparison_exception& ex) {
        out << "Check results: \n";
        //printout
        out << "Send memory:" << std::endl;
        memory_storage.dump_by_index(out, 0 /*send_mem*/);
        out << std::endl;
        out << "Recv memory:" << std::endl;
        memory_storage.dump_by_index(out, 1 /*recv_mem*/);
        out << std::endl;

        std::stringstream ss;
        ss << ex.what() << std::endl;
        UT_ASSERT(false, ss.str());
    }
}

} // namespace ring_single_device_case
