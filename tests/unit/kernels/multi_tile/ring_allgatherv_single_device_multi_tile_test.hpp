#pragma once

#include <memory>
#include <sstream>

#include "allgatherv_fixture.hpp"
#include "multi_tile_utils.hpp"

DEFINE_KERNEL_TYPES(allgatherv)

namespace ring_single_device_multi_tile_case {

TYPED_TEST_CASE(ring_allgatherv_single_process_fixture, TestTypes);
TYPED_TEST(ring_allgatherv_single_process_fixture, ring_allgatherv_single_device_multi_tile) {
    using namespace native;

    // Type of our test
    using native_type = TypeParam;

    // check test preconditions
    const auto& subdevices = this->get_local_devices();
    UT_ASSERT(
        subdevices.size() > 1,
        "Subdevices count: " << subdevices.size() << " should be more than 1 for multitile case");

    UT_ASSERT(subdevices.size() == this->get_unique_local_devices().size(),
              "Devices must be unique to launch multi tile case");

    // declare test case data
    const size_t comm_size = subdevices.size();
    const size_t send_buffer_base_size = 128;
    const size_t recv_buffer_size = send_buffer_base_size * (comm_size / 2) * (1 + comm_size);
    constexpr size_t comm_group_count = 3;
    constexpr size_t mem_group_count = 2;
    constexpr size_t flag_group_count = 2;
    std::map<size_t, std::vector<ccl_device::device_memory<size_t>>> comm_param_mem_storage;

    // device memory stencil data
    std::shared_ptr<ccl_context> ctx;
    std::vector<native_type> send_values(recv_buffer_size);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<native_type> recv_values(recv_buffer_size, 0);

    std::vector<size_t> recv_counts(comm_size, 0);
    std::vector<size_t> recv_offsets(comm_size, 0);
    for (size_t idx = 0; idx < comm_size; idx++) {
        recv_counts[idx] = send_buffer_base_size * (idx + 1);
        if (idx > 0)
            recv_offsets[idx] += recv_offsets[idx - 1] + recv_counts[idx - 1];
    }

    alloc_and_fill_allgatherv_buffers<native_type>(
        this, recv_counts, recv_offsets, subdevices, ctx);

    int rank = 0;
    for (auto dev_it = subdevices.begin(); dev_it != subdevices.end(); ++dev_it) {
        try {
            std::shared_ptr<ccl_device> sub_device = *dev_it;
            // initialize communication params
            this->output << "Create device memory & flags handles for device by index: "
                         << std::to_string(sub_device->get_device_id()) << ", as rank: (" << rank
                         << "/" << comm_size << ")" << std::endl;

            this->register_shared_comm_data(rank, rank, comm_size, recv_counts[rank]);

            auto mem_recv_counts = sub_device->alloc_memory<size_t>(comm_size, sizeof(size_t), ctx);
            auto mem_recv_offsets =
                sub_device->alloc_memory<size_t>(comm_size, sizeof(size_t), ctx);

            mem_recv_counts.enqueue_write_sync(recv_counts);
            mem_recv_offsets.enqueue_write_sync(recv_offsets);

            comm_param_mem_storage[rank].emplace_back(std::move(mem_recv_counts));
            comm_param_mem_storage[rank].emplace_back(std::move(mem_recv_offsets));

            //allocate flags & memory
            ze_device_mem_alloc_desc_t mem_uncached_descr{
                .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
                .pNext = NULL,
                .flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED,
                .ordinal = 0,
            };

            // flags
            auto left_wrote_2_me_flag =
                sub_device->alloc_memory<int>(1, sizeof(int), ctx, mem_uncached_descr);
            auto ready_for_receive_flag =
                sub_device->alloc_memory<int>(1, sizeof(int), ctx, mem_uncached_descr);
            left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
            ready_for_receive_flag.enqueue_write_sync({ (int)0 });

            /* fill array in specific order
             * Left: l_L, l_R, r_L, r_R
             * Right: r_L, r_R, l_L, L_R
             */
            this->register_shared_flags_data(
                rank, std::move(left_wrote_2_me_flag), std::move(ready_for_receive_flag));
            rank++;
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false,
                      "Cannot allocate memory for rank: " << rank << "\nError: " << ex.what());
        }
    }

    this->finalize_data_registration(comm_group_count, mem_group_count, flag_group_count);

    // prepare kernels
    for (size_t device_index = 0; device_index < subdevices.size(); device_index++) {
        this->create_kernel(device_index, allgatherv_param_traits<native_type>::kernel_name);
    }

    // prepare queues & lists
    std::map<size_t, ccl_device::device_queue> rank_queues;
    std::map<size_t, ccl_device::device_cmd_list> rank_cmd_lists;
    multi_tile_utils::prepare_queues_and_lists(
        subdevices, ctx, rank_queues, rank_cmd_lists, this->output);

    //printout memory handles
    this->dump_memory(this->output, true);

    // launch kernel for each device in separate thread
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    for (auto dev_it = subdevices.begin(); dev_it != subdevices.end(); ++dev_it) {
        ccl_device& subdevice = *(*dev_it);
        size_t rank = thread_group.size();
        ze_kernel_handle_t kernel = this->get_kernel(rank);
        auto& mem_handles = this->get_memory_handles(rank);
        auto& flag_handles = this->get_flag_handles(rank);
        auto& comm_handles = this->get_comm_handles(rank);
        auto& comm_mem_handles = find_storage_val(comm_param_mem_storage, rank);

        this->output << "Launch kernel params: \n"
                     << " subdevice id: " << ccl::to_string(subdevice.get_device_path())
                     << ", rank: " << rank << std::endl;

        ccl_device::device_queue& queue = rank_queues.find(rank)->second;
        ccl_device::device_cmd_list& list = rank_cmd_lists.find(rank)->second;

        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();
        thread_group.emplace_back([this,
                                   rank,
                                   kernel,
                                   &list,
                                   &queue,
                                   &mem_handles,
                                   &flag_handles,
                                   &comm_handles,
                                   &comm_mem_handles,
                                   raw_out]() {
            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { 1, 1, 1 };
            try {
                out << "Binding kernels arguments for rank: " << rank << std::endl;
                // bind rank, size
                out << "rank: " << rank << " - "
                    << "comm_offset" << std::endl;
                std::array<int, 3> comm_offset{ 0, 1, 2 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                bind_kernel_args(kernel, rank, comm_offset, comm_handles);

                // bind recv_counts, recv_offets
                out << "rank: " << rank << " - "
                    << "comm_mem_offset" << std::endl;
                std::array<int, 2> comm_mem_offset{ 3, 4 };
                bind_kernel_args(kernel, rank, comm_mem_offset, comm_mem_handles);

                // bind l_send, l_recv, r_recv
                out << "rank: " << rank << " - "
                    << "mem_offset" << std::endl;
                std::array<int, mem_group_count * 2> mem_offset{ 5, 6, -1, 7 };
                bind_kernel_args(kernel, rank, mem_offset, mem_handles);

                // bind left_wrote_2_me_flag, ready_for_receive_flag
                out << "rank: " << rank << " - "
                    << "flag_offset" << std::endl;
                std::array<int, flag_group_count * 2> flag_offset{ 8, 9, 10, 11 };
                bind_kernel_args(kernel, rank, flag_offset, flag_handles);

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
                UT_ASSERT(false, "Exception in rank: " << rank << "\nError: " << ex.what());
                throw;
            }
        });
        thread_out_put.push_back(std::move(out_ptr));
    }

    // waiting for threads with kernel execution
    size_t index = 0;
    for (auto& t : thread_group) {
        t.join();
        this->output << thread_out_put[index]->str() << std::endl;
        index++;
    }

    check_allgatherv_buffers<native_type>(this, comm_size, recv_counts);
}
} // namespace ring_single_device_multi_tile_case
