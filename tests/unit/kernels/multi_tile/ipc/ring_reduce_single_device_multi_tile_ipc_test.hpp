#pragma once

#include <memory>
#include <sstream>

#include "reduce_fixture.hpp"
#include "multi_tile_utils.hpp"

DEFINE_KERNEL_TYPES_FOR_OP(reduce, sum);
DEFINE_KERNEL_TYPES_FOR_OP(reduce, prod);
DEFINE_KERNEL_TYPES_FOR_OP(reduce, min);
DEFINE_KERNEL_TYPES_FOR_OP(reduce, max);

namespace ring_single_device_multi_tile_ipc_case {

TYPED_TEST_CASE(ring_reduce_multi_process_fixture, TestTypesAndOps);
TYPED_TEST(ring_reduce_multi_process_fixture, ring_reduce_single_device_multi_tile_ipc) {
    using namespace native;

    std::shared_ptr<ccl_context> ctx;

    // Type of our test
    using native_type = typename TypeParam::first_type;
    using op_type = typename TypeParam::second_type;

    // declare test case data
    const size_t buffer_size = 512;
    constexpr size_t comm_group_count = 4;
    constexpr size_t mem_group_count = 3;
    constexpr size_t ipc_mem_group_count = 1;
    constexpr size_t flag_group_count = 3;
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

    // declare test case data
    int world_size = 0;
    const auto total_dev_indices = this->get_cluster_platform_device_indices();
    for (const auto& process_indices : total_dev_indices) {
        const ccl::process_device_indices_type& thread_indices = process_indices.second;
        for (const auto& thread_index : thread_indices) {
            world_size += thread_index.second.size();
        }
    }

    int root = 1;

    alloc_and_fill_reduce_buffers<native_type>(this, buffer_size, devices, ctx, true);

    int rank_device_idx = 0;
    size_t device_index_start_offset =
        this->is_child() * devices.size(); /* global cluster numeration */
    this->output << "PID: " << this->pid << " calculated world size: " << world_size
                 << ", device_index_start_offset: " << device_index_start_offset << std::endl;

    for (const std::shared_ptr<ccl_device>& device : devices) {
        try {
            //initialize communication params
            int rank = rank_device_idx + device_index_start_offset;
            int comm_size = world_size;
            size_t elem_count = buffer_size;
            this->output << "Create device memory & flags handles for device by index: "
                         << std::to_string(device->get_device_id()) << ", as rank: ("
                         << rank_device_idx << "/" << comm_size << ")" << std::endl;

            this->register_shared_comm_data(rank_device_idx, rank, comm_size, elem_count, root);

            // allocate flags & memory
            ze_device_mem_alloc_desc_t mem_uncached_descr{
                .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
                .pNext = NULL,
                .flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED,
                .ordinal = 0,
            };

            // flags
            auto left_wrote_2_me_flag =
                device->alloc_memory<int>(1, sizeof(int), ctx, mem_uncached_descr);
            auto read_for_receive_flag =
                device->alloc_memory<int>(1, sizeof(int), ctx, mem_uncached_descr);
            auto barrier_flag = device->alloc_memory<int>(1, sizeof(int), ctx);
            left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
            read_for_receive_flag.enqueue_write_sync({ (int)0 });
            barrier_flag.enqueue_write_sync({ (int)0 });

            /* fill array in specific order
             * Left: l_L, l_R, l_B, r_L, r_R
             * Right: r_L, r_R, r_B, l_L, L_R
             */
            this->register_ipc_flags_data(
                ctx, rank_device_idx, &left_wrote_2_me_flag, &read_for_receive_flag);
            this->register_shared_flags_data(rank_device_idx,
                                             std::move(left_wrote_2_me_flag),
                                             std::move(read_for_receive_flag),
                                             std::move(barrier_flag));

            rank_device_idx++;
        }
        catch (const std::exception& ex) {
            UT_ASSERT(
                false,
                "Cannot allocate memory for rank: " << rank_device_idx << "\nError: " << ex.what());
        }
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
        this->create_kernel(device_index, reduce_param_traits<native_type, op_type>::kernel_name);
    }

    // prepare queues & lists
    std::map<size_t, ccl_device::device_queue> rank_queues;
    std::map<size_t, ccl_device::device_cmd_list> rank_cmd_lists;
    multi_tile_utils::prepare_queues_and_lists(
        devices, ctx, rank_queues, rank_cmd_lists, this->output);

    //printout memory handles
    this->dump_memory(this->output, true);

    // launch kernel for each device in separate thread
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    for (const auto& device : devices) {
        size_t rank = thread_group.size();
        ze_kernel_handle_t kernel = this->get_kernel(rank);
        auto& mem_handles = this->get_memory_handles(rank);
        auto& flag_handles = this->get_flag_handles(rank);
        auto& comm_handles = this->get_comm_handles(rank);

        this->output << "Launch kernel params: \n"
                     << " subdevice id: " << ccl::to_string(device->get_device_path())
                     << ", rank: " << rank_device_idx << std::endl;

        ccl_device::device_queue& queue = rank_queues.find(rank)->second;
        ccl_device::device_cmd_list& list = rank_cmd_lists.find(rank)->second;
        //ccl_device::device_cmd_list& list = rank_cmd_lists.find(0)->second;

        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();
        native::ccl_device* device_ptr = device.get();
        thread_group.emplace_back([this,
                                   rank,
                                   kernel,
                                   device_ptr,
                                   &list,
                                   &queue,
                                   &mem_handles,
                                   &flag_handles,
                                   &comm_handles,
                                   &ipc_client_flags,
                                   &ipc_client_memory,
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
                if (zeKernelSetGroupSize(
                        kernel, groupSizeX, groupSizeY, groupSizeZ != ZE_RESULT_SUCCESS)) {
                    throw std::runtime_error(std::string("Cannot set kernel group size, error"));
                }

                out << "Binding kernels arguments for rank: " << rank << std::endl;
                out << "rank: " << rank << " - "
                    << "comm_offset" << std::endl;
                // bind rank, size, buffer_size
                std::array<int, 4> comm_offset{ 0, 1, 2, 12 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                bind_kernel_args(kernel, rank, comm_offset, comm_handles);

                out << "rank: " << rank << " - "
                    << "mem_offset" << std::endl;
                // bind l_send, l_recv, l_tmp, , , r_tmp
                std::array<int, mem_group_count> mem_offset{ 3, 4, 5 };
                bind_kernel_args(kernel, rank, mem_offset, mem_handles);

                // Bind IPC memory
                std::array<int, ipc_mem_group_count> ipc_mem_offset{ 9 };
                out << "PID: " << this->pid << ", rank: " << rank << ", ipc_mem_handles: \n";
                bind_kernel_args(kernel, rank, ipc_mem_offset, ipc_mem_handles);

                out << "rank: " << rank << " - "
                    << "flag_offset" << std::endl;
                // bind left_wrote_2_me_flag, read_for_receive_flag, local_barrier_flag
                std::array<int, flag_group_count> flag_offset{ 6, 7, 8 };
                bind_kernel_args(kernel, rank, flag_offset, flag_handles);

                // Bind IPC flags
                std::array<int, ipc_flag_group_count> ipc_flag_offset{ 10, 11 };
                out << "PID: " << this->pid << ", rank: " << rank << ", ipc_flag_handles: \n"
                    << std::endl;
                bind_kernel_args(kernel, rank, ipc_flag_offset, ipc_flag_handles);

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
                          "Exception in rank: " << rank << "\nError: " << ex.what()
                                                << ", at phase: " << out.str());
                throw;
            }
        });

        rank++;
        thread_out_put.push_back(std::move(out_ptr));
    }

    size_t index = 0;
    for (auto& t : thread_group) {
        t.join();
        this->output << thread_out_put[index]->str() << std::endl;
        index++;
    }

    check_reduce_buffers<native_type, op_type>(this, world_size, buffer_size, root);

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
}
} // namespace ring_single_device_multi_tile_ipc_case
