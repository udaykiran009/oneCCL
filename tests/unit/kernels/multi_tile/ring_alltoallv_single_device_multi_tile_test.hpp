#pragma once

#include <memory>
#include <sstream>
#include "alltoallv_fixture.hpp"
#include "multi_tile_utils.hpp"

DEFINE_KERNEL_TYPES(alltoallv)

namespace ring_single_device_multi_tile_case {

TYPED_TEST_CASE(ring_alltoallv_single_process_fixture, TestTypes);
TYPED_TEST(ring_alltoallv_single_process_fixture, ring_alltoallv_single_device_multi_tile) {
    using namespace native;

    std::shared_ptr<ccl_context> ctx;

    // Type of our test
    using native_type = TypeParam;

    // test case data
    const auto& subdevices = this->get_local_devices();
    UT_ASSERT(
        subdevices.size() > 1,
        "Subdevices count: " << subdevices.size() << " should be more than 1 for multitile case");

    UT_ASSERT(subdevices.size() == this->get_unique_local_devices().size(),
              "Devices must be unique to launch multi tile case");

    // declare test case data
    const size_t comm_size = subdevices.size();
    const size_t send_buffer_base_size = 128;
    const size_t total_recv_size = send_buffer_base_size * (comm_size / 2) * (1 + comm_size);
    constexpr size_t comm_group_count = 2;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;

    std::vector<size_t> total_send_sizes(comm_size);
    for (size_t i = 0; i < comm_size; ++i) {
        total_send_sizes[i] = comm_size * send_buffer_base_size * (i + 1);
    }
    size_t tmp_buffer_size = send_buffer_base_size * comm_size * (comm_size - 1);

    std::map<size_t, std::vector<ccl_device::device_memory<size_t>>> comm_param_mem_storage;

    // send
    std::vector<std::vector<native_type>> send_values(comm_size);
    std::vector<std::vector<size_t>> send_counts(comm_size);
    std::vector<std::vector<size_t>> send_offsets(comm_size);

    size_t iota_base_value = 0;
    for (size_t idx = 0; idx < comm_size; idx++) {
        send_values[idx].resize(total_send_sizes[idx]);

        send_counts[idx].resize(comm_size, 0);
        send_offsets[idx].resize(comm_size, 0);
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
    std::vector<std::vector<native_type>> recv_values(comm_size);
    std::vector<std::vector<size_t>> recv_counts(comm_size);
    std::vector<std::vector<size_t>> recv_offsets(comm_size);
    for (size_t idx = 0; idx < comm_size; idx++) {
        recv_values[idx].resize(total_recv_size, 0);
        recv_counts[idx].resize(comm_size, 0);
        recv_offsets[idx].resize(comm_size, 0);
        for (size_t iter = 0; iter < recv_counts[idx].size(); iter++) {
            recv_counts[idx][iter] = send_buffer_base_size * (iter + 1);
            if (iter > 0)
                recv_offsets[idx][iter] += recv_offsets[idx][iter - 1] + recv_counts[idx][iter - 1];
        }
    }

    alloc_and_fill_alltoallv_buffers<native_type>(this,
                                                  total_send_sizes,
                                                  total_recv_size,
                                                  tmp_buffer_size,
                                                  send_values,
                                                  recv_values,
                                                  subdevices,
                                                  ctx);

    int rank = 0;
    for (auto dev_it = subdevices.begin(); dev_it != subdevices.end(); ++dev_it) {
        try {
            std::shared_ptr<ccl_device> sub_device = *dev_it;

            // initialize communication params
            this->output << "Create device memory & flags handles for device by index: "
                         << std::to_string(sub_device->get_device_id()) << ", as rank: (" << rank
                         << "/" << comm_size << ")" << std::endl;

            this->register_shared_comm_data(rank, rank, comm_size);

            // allocate flags & memory
            ze_device_mem_alloc_desc_t mem_uncached_descr{
                .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
                .pNext = NULL,
                .flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED,
                .ordinal = 0,
            };

            //allocate flags & memory
            // memory
            this->output << "Alloc memory handles: " << std::endl;
            // send
            auto mem_send_counts = sub_device->alloc_memory<size_t>(comm_size, sizeof(size_t), ctx);
            auto mem_send_offsets =
                sub_device->alloc_memory<size_t>(comm_size, sizeof(size_t), ctx);

            mem_send_counts.enqueue_write_sync(send_counts[rank]);
            mem_send_offsets.enqueue_write_sync(send_offsets[rank]);

            comm_param_mem_storage[rank].emplace_back(std::move(mem_send_counts));
            comm_param_mem_storage[rank].emplace_back(std::move(mem_send_offsets));

            // recv
            auto mem_recv_counts = sub_device->alloc_memory<size_t>(comm_size, sizeof(size_t), ctx);
            auto mem_recv_offsets =
                sub_device->alloc_memory<size_t>(comm_size, sizeof(size_t), ctx);

            mem_recv_counts.enqueue_write_sync(recv_counts[rank]);
            mem_recv_offsets.enqueue_write_sync(recv_offsets[rank]);

            comm_param_mem_storage[rank].emplace_back(std::move(mem_recv_counts));
            comm_param_mem_storage[rank].emplace_back(std::move(mem_recv_offsets));

            // flags
            auto left_wrote_2_me_flag =
                sub_device->alloc_memory<int>(1, sizeof(int), ctx, mem_uncached_descr);
            auto ready_for_receive_flag =
                sub_device->alloc_memory<int>(1, sizeof(int), ctx, mem_uncached_descr);
            auto proxy_size =
                sub_device->alloc_memory<int>(1, sizeof(int), ctx, mem_uncached_descr);
            left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
            ready_for_receive_flag.enqueue_write_sync({ (int)0 });
            proxy_size.enqueue_write_sync({ (int)0 });

            /* fill array in specific order
             * Left: l_L, l_R, r_L, r_R
             * Right: r_L, r_R, l_L, L_R
             */
            this->register_shared_flags_data(rank,
                                             std::move(left_wrote_2_me_flag),
                                             std::move(ready_for_receive_flag),
                                             std::move(proxy_size));

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
        this->create_kernel(device_index, alltoallv_param_traits<native_type>::kernel_name);
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
    for (const auto& subdevice : subdevices) {
        (void)subdevice;
        size_t rank = thread_group.size();
        ze_kernel_handle_t kernel = this->get_kernel(rank);
        auto& mem_handles = this->get_memory_handles(rank);
        auto& flag_handles = this->get_flag_handles(rank);
        auto& comm_handles = this->get_comm_handles(rank);
        auto& comm_mem_handles = find_storage_val(comm_param_mem_storage, rank);

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
                std::array<int, 2> comm_offset{ 0, 1 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                bind_kernel_args(kernel, rank, comm_offset, comm_handles);

                // bind recv_counts, recv_offets
                out << "rank: " << rank << " - "
                    << "comm_mem_offset" << std::endl;
                std::array<int, 4> comm_mem_offset{ 2, 3, 4, 5 };
                bind_kernel_args(kernel, rank, comm_mem_offset, comm_mem_handles);

                // bind l_send, l_recv, r_recv
                out << "rank: " << rank << " - "
                    << "mem_offset" << std::endl;
                std::array<int, mem_group_count * 2> mem_offset{ 6, 7, 8, -1, -1, 9 };
                bind_kernel_args(kernel, rank, mem_offset, mem_handles);

                // bind left_wrote_2_me_flag, ready_for_receive_flag
                out << "rank: " << rank << " - "
                    << "flag_offset" << std::endl;
                std::array<int, flag_group_count * 2> flag_offset{ 10, 11, 12, 13, 14, 15 };
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

    size_t index = 0;
    for (auto& t : thread_group) {
        t.join();
        this->output << thread_out_put[index]->str() << std::endl;
        index++;
    }

    check_alltoallv_buffers<native_type>(this, total_recv_size);
}
} // namespace ring_single_device_multi_tile_case
