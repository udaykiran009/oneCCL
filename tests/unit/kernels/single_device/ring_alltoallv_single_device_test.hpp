#pragma once

#include <memory>
#include <sstream>
#include "alltoallv_fixture.hpp"
#include "single_device_utils.hpp"

DEFINE_KERNEL_TYPES(alltoallv)

namespace ring_single_device_case {

namespace ring_alltoallv_case {}

TYPED_TEST_CASE(ring_alltoallv_single_process_fixture, TestTypes);

TYPED_TEST(ring_alltoallv_single_process_fixture, ring_alltoallv_single_device_mt) {
    using namespace native;

    std::shared_ptr<ccl_context> ctx;

    // Type of our test
    using native_type = TypeParam;

    // check test preconditions
    const auto& devices = this->get_local_devices();
    UT_ASSERT(devices.size() > 0,
              "SingleDevice test scope require at least 1 device in local platform! Use correct \""
                  << ut_device_affinity_mask_name << "\"");

    UT_ASSERT(this->get_unique_local_devices().size() == 1,
              "Devices must not be unique to launch single device case");

    // test case data
    const size_t comm_size = devices.size();
    constexpr size_t comm_group_count = 2;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;

    const size_t send_buffer_base_size = 128;
    size_t total_recv_size = send_buffer_base_size * (comm_size / 2) * (1 + comm_size);
    std::vector<size_t> total_send_sizes(comm_size);
    for (size_t i = 0; i < comm_size; ++i) {
        total_send_sizes[i] = comm_size * send_buffer_base_size * (i + 1);
    }
    size_t tmp_buffer_size = send_buffer_base_size * comm_size * (comm_size - 1);

    std::map<size_t, std::vector<ccl_device::device_memory<size_t>>> comm_param_mem_storage;

    // device memory stencil data
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
                                                  devices,
                                                  ctx);

    for (size_t rank = 0; rank < comm_size; rank++) {
        // initialize communication params
        this->output << "device id: " << ccl::to_string(devices[rank]->get_device_path())
                     << ", rank: " << rank << std::endl;

        this->register_shared_comm_data(rank, rank, comm_size);

        // send
        auto mem_send_counts =
            devices[rank]->template alloc_memory<size_t>(comm_size, sizeof(size_t), ctx);
        auto mem_send_offsets =
            devices[rank]->template alloc_memory<size_t>(comm_size, sizeof(size_t), ctx);

        mem_send_counts.enqueue_write_sync(send_counts[rank]);
        mem_send_offsets.enqueue_write_sync(send_offsets[rank]);

        comm_param_mem_storage[rank].emplace_back(std::move(mem_send_counts));
        comm_param_mem_storage[rank].emplace_back(std::move(mem_send_offsets));

        // recv
        auto mem_recv_counts =
            devices[rank]->template alloc_memory<size_t>(comm_size, sizeof(size_t), ctx);
        auto mem_recv_offsets =
            devices[rank]->template alloc_memory<size_t>(comm_size, sizeof(size_t), ctx);

        mem_recv_counts.enqueue_write_sync(recv_counts[rank]);
        mem_recv_offsets.enqueue_write_sync(recv_offsets[rank]);

        comm_param_mem_storage[rank].emplace_back(std::move(mem_recv_counts));
        comm_param_mem_storage[rank].emplace_back(std::move(mem_recv_offsets));

        // flags
        auto left_wrote_2_me_flag = devices[rank]->template alloc_memory<int>(1, sizeof(int), ctx);
        auto ready_for_receive_flag =
            devices[rank]->template alloc_memory<int>(1, sizeof(int), ctx);
        auto proxy_size = devices[rank]->template alloc_memory<int>(1, sizeof(int), ctx);
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
    }

    this->finalize_data_registration(comm_group_count, mem_group_count, flag_group_count);

    // prepare kernels
    for (size_t device_index = 0; device_index < comm_size; device_index++) {
        this->create_kernel(device_index, alltoallv_param_traits<native_type>::kernel_name);
    }

    // prepare queues & lists
    std::map<size_t, ccl_device::device_queue> rank_queues;
    std::map<size_t, ccl_device::device_cmd_list> rank_cmd_lists;

    single_device_utils::prepare_kernel_queues_lists(
        devices, ctx, rank_queues, rank_cmd_lists, this->output);

    //printout memory handles
    this->dump_memory(this->output, true);

    //Set args and launch kernel
    std::mutex thread_lock; //workaround
    std::atomic<size_t> val{ 0 }; //workaround
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    for (size_t rank = 0; rank < comm_size; rank++) {
        ze_kernel_handle_t kernel = this->get_kernel(rank);
        auto& mem_handles = this->get_memory_handles(rank);
        auto& flag_handles = this->get_flag_handles(rank);
        auto& comm_handles = this->get_comm_handles(rank);
        auto& comm_mem_handles = find_storage_val(comm_param_mem_storage, rank);

        //WORKAROUND: ONLY ONE LIST & QUEUE!
        ccl_device::device_queue& queue = rank_queues.find(0)->second;
        ccl_device::device_cmd_list& list = rank_cmd_lists.find(0)->second;

        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();
        thread_group.emplace_back([this,
                                   rank,
                                   kernel,
                                   comm_size,
                                   &list,
                                   &queue,
                                   &mem_handles,
                                   &flag_handles,
                                   &comm_handles,
                                   &comm_mem_handles,
                                   &thread_lock,
                                   &val,
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
                while (val < comm_size) {
                }

                // let thread 0 to be the one submitting commands to the queue and sync
                if (rank == 0) {
                    queue_sync_processing(list, queue);
                    out << "thread finished" << std::endl;
                }
            }
            catch (const std::exception& ex) {
                UT_ASSERT(false,
                          "Exception in rank: " << rank << "\nError: " << ex.what()
                                                << ", at phase: " << out.str());
                throw;
            }
        });

        thread_out_put.push_back(std::move(out_ptr));
    }

    size_t index = 0;
    for (auto& t : thread_group) {
        t.join();
        this->output << "Kernels argument binding log for rank: " << index << std::endl;
        this->output << thread_out_put[index]->str() << std::endl;
        index++;
    }

    check_alltoallv_buffers<native_type>(this, total_recv_size);
}

} // namespace ring_single_device_case
