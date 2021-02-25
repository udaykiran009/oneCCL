#pragma once

#include <memory>
#include <sstream>

#include "allreduce_fixture.hpp"
#include "multi_tile_utils.hpp"

DEFINE_KERNEL_TYPES_FOR_OP_BF16(allreduce, sum);
DEFINE_KERNEL_TYPES_FOR_OP_BF16(allreduce, prod);
DEFINE_KERNEL_TYPES_FOR_OP_BF16(allreduce, min);
DEFINE_KERNEL_TYPES_FOR_OP_BF16(allreduce, max);

namespace ring_single_device_multi_tile_ipc_case {

TYPED_TEST_CASE(ring_allreduce_multi_process_fixture, TestTypesAndOpsReduction);
TYPED_TEST(ring_allreduce_multi_process_fixture, ring_single_device_multi_tile_ipc) {
    using namespace native;
    using namespace net;

    std::shared_ptr<ccl_context> ctx;

    // Type of our test
    using native_type = typename TypeParam::first_type;
    using op_type = typename TypeParam::second_type;

    // test case data
    const size_t buffer_size = 512;
    constexpr size_t comm_group_count = 3;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;
    constexpr size_t ipc_mem_group_count = 1;
    constexpr size_t ipc_flag_group_count = 2;

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

    // START TEST execution

    alloc_and_fill_allreduce_buffers<native_type>(this, buffer_size, devices, ctx, true);

    // allocate & initialize test kernel params
    int rank_device_idx = 0;
    int comm_size = 0;
    const auto total_dev_indices = this->get_cluster_platform_device_indices();
    for (const auto& process_indices : total_dev_indices) {
        const ccl::process_device_indices_type& thread_indices = process_indices.second;
        for (const auto& thread_index : thread_indices) {
            comm_size += thread_index.second.size();
        }
    }

    size_t device_index_start_offset =
        this->is_child() * devices.size(); /* global cluster numeration */
    this->output << "PID: " << this->pid << " calculated world size: " << comm_size
                 << ", device_index_start_offset: " << device_index_start_offset << std::endl;
    for (const std::shared_ptr<ccl_device>& device : devices) {
        try {
            // initialize communication params
            int rank = rank_device_idx + device_index_start_offset;
            size_t elem_count = buffer_size;

            this->register_shared_comm_data(rank_device_idx, rank, comm_size, elem_count);

            // flags
            auto left_wrote_2_me_flag = device->alloc_memory<int>(1, sizeof(int), ctx);
            auto read_for_receive_flag = device->alloc_memory<int>(1, sizeof(int), ctx);
            auto barrier_flag = device->alloc_memory<int>(1, sizeof(int), ctx);
            left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
            read_for_receive_flag.enqueue_write_sync({ (int)0 });
            barrier_flag.enqueue_write_sync({ (int)0 });

            this->register_ipc_flags_data(
                ctx, rank_device_idx, &left_wrote_2_me_flag, &read_for_receive_flag);

            /* fill array in specific order
             * Left: l_L, l_R, l_B, r_L, r_R
             * Right: r_L, r_R, r_B, l_L, L_R
             */
            this->register_shared_flags_data(rank_device_idx,
                                             std::move(left_wrote_2_me_flag),
                                             std::move(read_for_receive_flag),
                                             std::move(barrier_flag));
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
        this->create_kernel(device_index,
                            allreduce_param_traits<native_type, op_type>::ipc_kernel_name);
    }

    // prepare queues & lists
    this->output << "PID: " << this->pid << "Prepare queue and lists \n";
    std::map<size_t, ccl_device::device_queue> rank_queues;
    std::map<size_t, ccl_device::device_cmd_list> rank_cmd_lists;
    multi_tile_utils::prepare_queues_and_lists(devices,
                                               ctx,
                                               rank_queues,
                                               rank_cmd_lists,
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
        size_t rank = thread_group.size();
        ze_kernel_handle_t kernel = this->get_kernel(rank);
        auto mem_handles = this->get_memory_handles(rank);
        auto flag_handles = this->get_flag_handles(rank);
        auto comm_handles = this->get_comm_handles(rank);

        ccl_device::device_queue& queue = rank_queues.find(rank)->second;
        ccl_device::device_cmd_list& list = rank_cmd_lists.find(rank)->second;

        this->output << "PID: " << this->pid << ", start thread for kernel execution" << std::endl;
        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();

        native::ccl_device* device_ptr = device.get();
        thread_group.emplace_back([this,
                                   rank,
                                   kernel,
                                   device_ptr,
                                   &list,
                                   &queue,
                                   mem_handles,
                                   flag_handles,
                                   &ipc_client_memory,
                                   &ipc_client_flags,
                                   comm_handles,
                                   raw_out]() {
            auto& ipc_mem_handles = ipc_client_memory.find(device_ptr)->second;
            auto& ipc_flag_handles = ipc_client_flags.find(device_ptr)->second;

            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { 1, 1, 1 };
            try {
                // set group size
                uint32_t groupSizeX = buffer_size;
                uint32_t groupSizeY = 1u;
                uint32_t groupSizeZ = 1u;
                if (zeKernelSetGroupSize(kernel, groupSizeX, groupSizeY, groupSizeZ)) {
                    throw std::runtime_error(std::string("Cannot set kernel group size, error"));
                }

                // bind rank, size, buffer_size
                out << "PID: " << this->pid << ", rank: " << rank << ", comm_handles: \n"
                    << std::endl;
                std::array<int, 3> comm_offset{ 0, 1, 2 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                bind_kernel_args(kernel, rank, comm_offset, comm_handles);

                // bind l_send, l_recv, l_tmp,
                out << "PID: " << this->pid << ", rank: " << rank << ", mem_handles: \n";
                std::array<int, mem_group_count> mem_offset{ 3, 4, 5 };
                bind_kernel_args(kernel, rank, mem_offset, mem_handles);

                // Bind IPC memory
                std::array<int, ipc_mem_group_count> ipc_mem_offset{ 9 };
                out << "PID: " << this->pid << ", rank: " << rank << ", ipc_mem_handles: \n";
                bind_kernel_args(kernel, rank, ipc_mem_offset, ipc_mem_handles);

                // bindleft_wrote_2_me_flag, read_for_receive_flag, local_barrier_flag
                std::array<int, flag_group_count> flag_offset{ 6, 7, 8 };
                out << "PID: " << this->pid << ", rank: " << rank << ", flag_handles: \n"
                    << std::endl;
                bind_kernel_args(kernel, rank, flag_offset, flag_handles);

                // Bind IPC flags
                std::array<int, ipc_flag_group_count> ipc_flag_offset{ 10, 11 };
                out << "PID: " << this->pid << ", rank: " << rank << ", ipc_flag_handles: \n"
                    << std::endl;
                bind_kernel_args(kernel, rank, ipc_flag_offset, ipc_flag_handles);

                ze_result_t ret = zeCommandListAppendLaunchKernel(
                    list.handle, kernel, &launch_args, nullptr, 0, nullptr);
                if (ret != ZE_RESULT_SUCCESS) {
                    throw std::runtime_error(
                        std::string("cannot zeCommandListAppendLaunchKernel, error: ") +
                        std::to_string(ret));
                }

                //no sync processes here
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
                UT_ASSERT(false,
                          "Exception in PID: " << this->pid << ", rank: " << rank
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

    check_allreduce_buffers<native_type, op_type>(this, comm_size, buffer_size);

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
