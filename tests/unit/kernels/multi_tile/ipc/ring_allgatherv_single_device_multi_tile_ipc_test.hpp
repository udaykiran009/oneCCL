#pragma once

#include <memory>
#include <sstream>

#include "allgatherv_fixture.hpp"
#include "multi_tile_utils.hpp"

DEFINE_KERNEL_TYPES(allgatherv)

namespace ring_single_device_multi_tile_ipc_case {

TYPED_TEST_CASE(ring_allgatherv_multi_process_fixture, TestTypes);
TYPED_TEST(ring_allgatherv_multi_process_fixture, ring_single_device_multi_tile_ipc) {
    using namespace native;
    using namespace net;

    // Type of our test
    using native_type = TypeParam;

    // check test preconditions
    const auto& devices = this->get_local_devices();
    const auto& global_devices = this->get_global_devices();
    UT_ASSERT(devices.size() >= 1,
              "Devices count: " << devices.size()
                                << " should be greater or equal than 1 for multitile ipc case");

    UT_ASSERT(global_devices.size() >= devices.size() * 2,
              "Global Devices count: " << global_devices.size()
                                       << " should be double from local devices count: "
                                       << devices.size());

    UT_ASSERT(devices.size() == this->get_unique_local_devices().size(),
              "Devices must be unique to launch multi device case");

    // declare test case data
    int rank_device_idx = 0;
    int world_size = 0;
    const auto total_dev_indices = this->get_cluster_platform_device_indices();
    for (const auto& process_indices : total_dev_indices) {
        const ccl::process_device_indices_type& thread_indices = process_indices.second;
        for (const auto& thread_index : thread_indices) {
            world_size += thread_index.second.size();
        }
    }
    const size_t send_buffer_base_size = 128;
    const size_t recv_buffer_size = send_buffer_base_size * (world_size / 2) * (1 + world_size);
    constexpr size_t comm_group_count = 3;
    constexpr size_t comm_param_group_count = 2;
    constexpr size_t mem_group_count = 2;
    constexpr size_t flag_group_count = 2;
    constexpr size_t ipc_mem_group_count = 1;
    constexpr size_t ipc_flag_group_count = 2;
    std::map<size_t, std::vector<ccl_device::device_memory<size_t>>> comm_param_mem_storage;

    // device memory stencil data
    std::shared_ptr<ccl_context> ctx;
    std::vector<native_type> send_values(recv_buffer_size);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<native_type> recv_values(recv_buffer_size, 0);

    std::vector<size_t> recv_counts(world_size, 0);
    std::vector<size_t> recv_offsets(world_size, 0);
    for (int idx = 0; idx < world_size; idx++) {
        recv_counts[idx] = send_buffer_base_size * (idx + 1);
        if (idx > 0)
            recv_offsets[idx] += recv_offsets[idx - 1] + recv_counts[idx - 1];
    }

    size_t device_index_start_offset =
        this->is_child() * devices.size(); /* global cluster numeration */
    this->output << "PID: " << this->pid << " calculated world size: " << world_size
                 << ", device_index_start_offset: " << device_index_start_offset << std::endl;
    for (const std::shared_ptr<ccl_device>& device : devices) {
        try {
            // initialize communication params
            int rank_idx = rank_device_idx + device_index_start_offset;
            size_t elem_count = recv_counts[rank_idx];

            this->register_shared_comm_data(rank_device_idx, rank_idx, world_size, elem_count);

            auto mem_recv_counts = device->alloc_memory<size_t>(world_size, sizeof(size_t), ctx);
            auto mem_recv_offsets = device->alloc_memory<size_t>(world_size, sizeof(size_t), ctx);

            mem_recv_counts.enqueue_write_sync(recv_counts);
            mem_recv_offsets.enqueue_write_sync(recv_offsets);

            comm_param_mem_storage[rank_idx].emplace_back(std::move(mem_recv_counts));
            comm_param_mem_storage[rank_idx].emplace_back(std::move(mem_recv_offsets));

            //allocate flags & memory
            ze_device_mem_alloc_desc_t mem_uncached_descr{
                .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
                .pNext = NULL,
                .flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED,
                .ordinal = 0,
            };
            // memory
            this->output << "Alloc memory handles: " << std::endl;
            auto mem_send = device->alloc_memory<native_type>(elem_count, sizeof(native_type), ctx);
            auto mem_recv =
                device->alloc_memory<native_type>(recv_buffer_size, sizeof(native_type), ctx);

            mem_send.enqueue_write_sync(send_values.begin() + recv_offsets[rank_idx],
                                        send_values.begin() + recv_offsets[rank_idx] + elem_count);

            mem_recv.enqueue_write_sync(recv_values.begin(),
                                        recv_values.begin() + recv_buffer_size);

            this->template register_ipc_memories_data<native_type>(ctx, rank_device_idx, &mem_recv);

            this->register_shared_memories_data(
                rank_device_idx, std::move(mem_send), std::move(mem_recv));

            // flags
            auto left_wrote_2_me_flag =
                device->alloc_memory<int>(1, sizeof(int), ctx, mem_uncached_descr);
            auto ready_for_receive_flag =
                device->alloc_memory<int>(1, sizeof(int), ctx, mem_uncached_descr);
            left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
            ready_for_receive_flag.enqueue_write_sync({ (int)0 });

            this->register_ipc_flags_data(
                ctx, rank_device_idx, &left_wrote_2_me_flag, &ready_for_receive_flag);

            /* fill array in specific order
             * Left: l_L, l_R, r_L, r_R
             * Right: r_L, r_R, l_L, L_R
             */
            this->register_shared_flags_data(rank_device_idx,
                                             std::move(left_wrote_2_me_flag),
                                             std::move(ready_for_receive_flag));
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false,
                      "Cannot allocate memory for device num: %" << rank_device_idx
                                                                 << "\nError: " << ex.what());
        }

        rank_device_idx++;
    }

    this->finalize_data_registration(comm_group_count, mem_group_count, flag_group_count);

    // Make IPC handles exchange
    this->output << "PID: " << *this->my_pid << " create ipc handle from memory handles"
                 << std::endl;
    auto ipc_client_memory = this->send_receive_ipc_memory(devices, ctx);
    auto ipc_client_flags = this->send_receive_ipc_flags(devices, ctx);
    this->output << "PID: " << *this->my_pid << " finishes IPC handles exchange" << std::endl;

    // prepare kernels
    for (size_t device_index = 0; device_index < devices.size(); device_index++) {
        this->create_kernel(device_index, allgatherv_param_traits<native_type>::ipc_kernel_name);
    }

    // prepare queues & lists
    this->output << "PID: " << this->pid << "Prepare queue and lists \n";
    std::map<size_t, ccl_device::device_queue> thread_queue;
    std::map<size_t, ccl_device::device_cmd_list> thread_cmd_list;
    multi_tile_utils::prepare_queues_and_lists(devices,
                                               ctx,
                                               thread_queue,
                                               thread_cmd_list,
                                               this->output,
                                               device_index_start_offset /* cluster offset */);

    //printout memory handles
    this->output << "PID: " << this->pid << "\n*************************\n";
    this->dump_memory(this->output, true);
    this->output << std::endl;

    //Set args and launch kernel
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    for (const auto& device : devices) {
        (void)device;
        size_t thread_idx = thread_group.size();
        ze_kernel_handle_t kernel = this->get_kernel(thread_idx);
        auto mem_handles = this->get_memory_handles(thread_idx);
        auto flag_handles = this->get_flag_handles(thread_idx);
        auto comm_handles = this->get_comm_handles(thread_idx);
        auto& comm_mem_handles = find_storage_val(comm_param_mem_storage, thread_idx);

        ccl_device::device_queue& queue = thread_queue.find(thread_idx)->second;
        ccl_device::device_cmd_list& list = thread_cmd_list.find(thread_idx)->second;

        this->output << "PID: " << this->pid << ", start thread for kernel execution" << std::endl;
        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();

        native::ccl_device* device_ptr = device.get();
        thread_group.emplace_back([this,
                                   thread_idx,
                                   kernel,
                                   device_ptr,
                                   &list,
                                   &queue,
                                   mem_handles,
                                   flag_handles,
                                   &ipc_client_memory,
                                   &ipc_client_flags,
                                   comm_handles,
                                   &comm_mem_handles,
                                   raw_out]() {
            auto& ipc_mem_handles = ipc_client_memory.find(device_ptr)->second;
            auto& ipc_flag_handles = ipc_client_flags.find(device_ptr)->second;

            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { 1, 1, 1 };
            try {
                // set group size
                /*
                uint32_t groupSizeX = buffer_size;
                uint32_t groupSizeY = 1u;
                uint32_t groupSizeZ = 1u;
                if (zeKernelSetGroupSize(
                        kernel, groupSizeX, groupSizeY, groupSizeZ != ZE_RESULT_SUCCESS)) {
                    throw std::runtime_error(std::string("Cannot set kernel group size, error"));
                }
                */
                // bind rank, size, buffer_size
                out << "PID: " << this->pid << ", thread_idx: " << thread_idx
                    << ", comm_handles: \n"
                    << std::endl;
                std::array<int, comm_group_count> comm_offset{ 0, 1, 2 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                bind_kernel_args(kernel, thread_idx, comm_offset, comm_handles);

                // bind recv_counts, recv_offets
                out << "thread_idx: " << thread_idx << " - "
                    << "comm_mem_offset" << std::endl;
                std::array<int, comm_param_group_count> comm_mem_offset{ 3, 4 };
                bind_kernel_args(kernel, thread_idx, comm_mem_offset, comm_mem_handles);

                // bind l_send, l_recv, r_recv
                out << "thread_idx: " << thread_idx << " - "
                    << "mem_offset" << std::endl;
                std::array<int, mem_group_count> mem_offset{ 5, 6 };
                bind_kernel_args(kernel, thread_idx, mem_offset, mem_handles);

                // Bind IPC memory
                std::array<int, ipc_mem_group_count> ipc_mem_offset{ 7 };
                out << "PID: " << this->pid << ", thread_idx: " << thread_idx
                    << ", ipc_mem_handles: \n";
                bind_kernel_args(kernel, thread_idx, ipc_mem_offset, ipc_mem_handles);

                // bind left_wrote_2_me_flag, ready_for_receive_flag
                out << "thread_idx: " << thread_idx << " - "
                    << "flag_offset" << std::endl;
                std::array<int, flag_group_count> flag_offset{ 8, 9 };
                bind_kernel_args(kernel, thread_idx, flag_offset, flag_handles);

                // Bind IPC flags
                std::array<int, ipc_flag_group_count> ipc_flag_offset{ 10, 11 };
                out << "PID: " << this->pid << ", thread_idx: " << thread_idx
                    << ", ipc_flag_handles: \n"
                    << std::endl;
                bind_kernel_args(kernel, thread_idx, ipc_flag_offset, ipc_flag_handles);

                ze_result_t ret = zeCommandListAppendLaunchKernel(
                    list.handle, kernel, &launch_args, nullptr, 0, nullptr);
                if (ret != ZE_RESULT_SUCCESS) {
                    throw std::runtime_error(
                        std::string("cannot zeCommandListAppendLaunchKernel, error: ") +
                        std::to_string(ret));
                }

                queue_sync_processing(list, queue);
                out << "thread finished" << std::endl;
            }
            catch (const std::exception& ex) {
                UT_ASSERT(false,
                          "Exception in PID: " << this->pid << ",thread: " << thread_idx
                                               << "\nError: " << ex.what() << ", at phase:\n{\n"
                                               << out.str() << "\n}\n");
                throw;
            }
        });

        thread_out_put.push_back(std::move(out_ptr));
    }

    size_t index = 0;
    this->output << "PID: " << this->pid << ", wating threads" << std::endl;
    for (auto& t : thread_group) {
        try {
            t.join();
            this->output << "PID: " << this->pid << "\n*************************\n"
                         << thread_out_put[index]->str() << "\n*************************\n"
                         << std::endl;
        }
        catch (const std::exception& ex) {
            this->output << "PID: " << this->pid << "\n******ERROR*******************\n"
                         << thread_out_put[index]->str() << "\n*************************\n"
                         << ex.what() << std::endl;
        }
        index++;
    }

    std::stringstream ss;
    bool ret = allgatherv_checking_results<native_type>(this, world_size, ss);
    UT_ASSERT(ret, ss.str());

    // gracefull finalize
    uint8_t ready = 0;
    if (this->is_child()) {
        utils::readFromSocket(this->communication_socket, &ready, sizeof(ready));
    }
    else {
        ready = 1;
        utils::writeToSocket(this->communication_socket, &ready, sizeof(ready));
    }
    this->output << "PID: " << this->pid << ", finished, status: " << ready << std::endl;
    quick_exit(0);
}

} // namespace ring_single_device_multi_tile_ipc_case
