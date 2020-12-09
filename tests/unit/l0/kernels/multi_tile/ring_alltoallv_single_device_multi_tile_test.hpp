#pragma once

#include <memory>
#include <sstream>
#include "../alltoallv_fixture.hpp"

DEFINE_KERNEL_TYPES(alltoallv)

namespace ring_single_device_multi_tile_case {

TYPED_TEST_CASE(ring_alltoallv_single_device_multi_tile_fixture, TestTypes);
TYPED_TEST(ring_alltoallv_single_device_multi_tile_fixture,
           ring_alltoallv_single_device_multi_tile) {
    using namespace native;

    // Type of our test
    using native_type = TypeParam;

    // test case data
    const int num_thread = 4;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;

    ze_device_mem_alloc_desc_t mem_descr{
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED,
        .ordinal = 0,
    };

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
    auto driver = drv_it->second;

    UT_ASSERT(driver->devices.empty() != true, "There are no available devices");
    ccl_device_driver::device_ptr one_device = driver->devices.begin()->second;
    auto& subdevices = one_device->get_subdevices();

    UT_ASSERT(subdevices.size() == num_thread,
              "Subdevices count: " << subdevices.size()
                                   << " should be equal with thread count: " << num_thread);

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

    int rank_device_idx = 0;
    for (auto dev_it = subdevices.begin(); dev_it != subdevices.end(); ++dev_it) {
        try {
            std::shared_ptr<ccl_subdevice> sub_device = dev_it->second;

            // initialize communication params
            int rank_idx = rank_device_idx;
            int rank_size = num_thread;

            this->output << "Create device memory & flags handles for device by index: "
                         << std::to_string(sub_device->get_device_id()) << ", as rank: ("
                         << rank_device_idx << "/" << rank_size << ")" << std::endl;

            comm_param_storage[rank_device_idx].push_back(rank_idx);
            comm_param_storage[rank_device_idx].push_back(rank_size);

            //allocate flags & memory
            // memory
            this->output << "Alloc memory handles: " << std::endl;
            // send
            auto mem_send_counts =
                sub_device->alloc_memory<size_t>(num_thread, sizeof(size_t), ctx);
            auto mem_send_offsets =
                sub_device->alloc_memory<size_t>(num_thread, sizeof(size_t), ctx);

            mem_send_counts.enqueue_write_sync(send_counts[rank_device_idx]);
            mem_send_offsets.enqueue_write_sync(send_offsets[rank_device_idx]);

            comm_param_mem_storage[rank_device_idx].emplace_back(std::move(mem_send_counts));
            comm_param_mem_storage[rank_device_idx].emplace_back(std::move(mem_send_offsets));

            // recv
            auto mem_recv_counts =
                sub_device->alloc_memory<size_t>(num_thread, sizeof(size_t), ctx);
            auto mem_recv_offsets =
                sub_device->alloc_memory<size_t>(num_thread, sizeof(size_t), ctx);

            mem_recv_counts.enqueue_write_sync(recv_counts[rank_device_idx]);
            mem_recv_offsets.enqueue_write_sync(recv_offsets[rank_device_idx]);

            comm_param_mem_storage[rank_device_idx].emplace_back(std::move(mem_recv_counts));
            comm_param_mem_storage[rank_device_idx].emplace_back(std::move(mem_recv_offsets));

            // allocate flags & memory
            // memory
            auto mem_send = sub_device->alloc_memory<native_type>(
                total_send_sizes[rank_device_idx], sizeof(native_type), ctx);
            auto mem_recv =
                sub_device->alloc_memory<native_type>(total_recv_size, sizeof(native_type), ctx);
            auto mem_tmp = sub_device->alloc_memory<native_type>(
                tmp_buffer_size, sizeof(native_type), ctx, mem_descr);

            mem_send.enqueue_write_sync(send_values[rank_device_idx]);
            mem_recv.enqueue_write_sync(recv_values[rank_device_idx]);
            mem_tmp.enqueue_write_sync(tmp_values);

            memory_storage.register_shared_data(rank_device_idx,
                                                num_thread,
                                                std::move(mem_send),
                                                std::move(mem_recv),
                                                std::move(mem_tmp));

            // flags
            auto left_wrote_2_me_flag =
                sub_device->alloc_memory<int>(1, sizeof(int), ctx, mem_descr);
            auto ready_for_receive_flag =
                sub_device->alloc_memory<int>(1, sizeof(int), ctx, mem_descr);
            auto proxy_size = sub_device->alloc_memory<int>(1, sizeof(int), ctx, mem_descr);
            left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
            ready_for_receive_flag.enqueue_write_sync({ (int)0 });
            proxy_size.enqueue_write_sync({ (int)0 });

            /* fill array in specific order
             * Left: l_L, l_R, r_L, r_R
             * Right: r_L, r_R, l_L, L_R
             */
            flags_storage.register_shared_data(rank_device_idx,
                                               num_thread,
                                               std::move(left_wrote_2_me_flag),
                                               std::move(ready_for_receive_flag),
                                               std::move(proxy_size));

            rank_device_idx++;
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false,
                      "Cannot allocate memory for thread: " << rank_device_idx
                                                            << "\nError: " << ex.what());
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
    this->output << "KERNEL_NAME: " << desc.pKernelName << std::endl;

    std::map<size_t, ze_kernel_handle_t> thread_kernels;
    std::map<size_t, ccl_device::device_queue> thread_queue;
    std::map<size_t, ccl_device::device_cmd_list> thread_cmd_list;

    rank_device_idx = 0;
    for (auto dev_it = subdevices.begin(); dev_it != subdevices.end(); ++dev_it) {
        ccl_subdevice& device = *(dev_it->second);
        ccl_device::device_module& module = *(this->device_modules.find(&device)->second);

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
    memory_storage.dump(this->output, true);

    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    size_t thread_idx = 0;
    for (auto dev_it = subdevices.begin(); dev_it != subdevices.end(); ++dev_it) {
        ccl_subdevice& subdevice = *(dev_it->second);
        ze_kernel_handle_t kernel = thread_kernels.find(thread_idx)->second;
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
                                   &subdevice,
                                   thread_idx,
                                   kernel,
                                   &list,
                                   &queue,
                                   &mem_handles,
                                   &flag_handles,
                                   &comm_handles,
                                   &comm_mem_handles,
                                   raw_out]() {
            (void)driver;
            (void)subdevice;

            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { 1, 1, 1 };
            try {
                out << "Binding kernels arguments for thread:" << thread_idx << std::endl;
                // bind rank, size
                out << "thread_idx: " << thread_idx << " - "
                    << "comm_offset" << std::endl;
                std::array<int, 2> comm_offset{ 0, 1 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                bind_kernel_args(kernel, thread_idx, comm_offset, comm_handles);

                // bind recv_counts, recv_offets
                out << "thread_idx: " << thread_idx << " - "
                    << "comm_mem_offset" << std::endl;
                std::array<int, 4> comm_mem_offset{ 2, 3, 4, 5 };
                bind_kernel_args(kernel, thread_idx, comm_mem_offset, comm_mem_handles);

                // bind l_send, l_recv, r_recv
                out << "thread_idx: " << thread_idx << " - "
                    << "mem_offset" << std::endl;
                std::array<int, mem_group_count * 2> mem_offset{ 6, 7, 8, -1, -1, 9 };
                bind_kernel_args(kernel, thread_idx, mem_offset, mem_handles);

                // bind left_wrote_2_me_flag, ready_for_receive_flag
                out << "thread_idx: " << thread_idx << " - "
                    << "flag_offset" << std::endl;
                std::array<int, flag_group_count * 2> flag_offset{ 10, 11, 12, 13, 14, 15 };
                bind_kernel_args(kernel, thread_idx, flag_offset, flag_handles);

                {
                    ze_result_t ret = ZE_RESULT_SUCCESS;
                    {
                        ret = zeCommandListAppendLaunchKernel(
                            list.handle, kernel, &launch_args, nullptr, 0, nullptr);
                        if (ret != ZE_RESULT_SUCCESS) {
                            throw std::runtime_error(
                                std::string("cannot zeCommandListAppendLaunchKernel, error: ") +
                                std::to_string(ret));
                        }
                    }
                }

                queue_sync_processing(list, queue);
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
        this->output << thread_out_put[index]->str() << std::endl;
        index++;
    }

    //printout
    memory_storage.dump(this->output);
}
} // namespace ring_single_device_multi_tile_case