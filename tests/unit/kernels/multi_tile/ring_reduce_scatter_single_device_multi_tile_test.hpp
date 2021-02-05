#pragma once

#include <memory>
#include <sstream>

#include "reduce_scatter_fixture.hpp"
#include "multi_tile_utils.hpp"

DEFINE_KERNEL_TYPES_FOR_OP(reduce_scatter, sum);
DEFINE_KERNEL_TYPES_FOR_OP(reduce_scatter, prod);
DEFINE_KERNEL_TYPES_FOR_OP(reduce_scatter, min);
DEFINE_KERNEL_TYPES_FOR_OP(reduce_scatter, max);

namespace ring_single_device_multi_tile_case {
TYPED_TEST_CASE(ring_reduce_scatter_single_process_fixture, TestTypesAndOps);

TYPED_TEST(ring_reduce_scatter_single_process_fixture,
           ring_reduce_scatter_single_device_multi_tile) {
    using namespace native;

    // Type of our test
    using native_type = typename TypeParam::first_type;
    using op_type = typename TypeParam::second_type;

    // check test preconditions
    const auto& subdevices = this->get_local_devices();
    UT_ASSERT(
        subdevices.size() > 1,
        "Subdevices count: " << subdevices.size() << " should be more than 1 for multitile case");

    UT_ASSERT(subdevices.size() == this->get_unique_local_devices().size(),
              "Devices must be unique to launch multi tile case");

    // declare test case data
    int num_thread = subdevices.size();
    const size_t recv_buffer_size = 64;
    const size_t send_buffer_size = num_thread * recv_buffer_size;
    constexpr size_t comm_group_count = 3;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;

    // fill device memory stencil data
    std::shared_ptr<ccl_context> ctx;
    std::vector<native_type> send_values(send_buffer_size);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<native_type> recv_values(send_buffer_size, 0);

    int rank_device_idx = 0;
    for (const std::shared_ptr<ccl_device>& sub_device : subdevices) {
        try {
            //initialize communication params
            int rank_idx = rank_device_idx;
            int rank_size = num_thread;
            size_t send_elem_count = send_buffer_size;

            this->output << "Create device memory & flags handles for device by index: "
                         << std::to_string(sub_device->get_device_id()) << ", as rank: ("
                         << rank_device_idx << "/" << rank_size << ")" << std::endl;
            this->register_shared_comm_data(rank_device_idx, rank_idx, rank_size, send_elem_count);

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
            auto mem_send =
                sub_device->alloc_memory<native_type>(send_buffer_size, sizeof(native_type), ctx);
            auto mem_recv =
                sub_device->alloc_memory<native_type>(recv_buffer_size, sizeof(native_type), ctx);
            auto temp_recv = sub_device->alloc_memory<native_type>(
                2 * recv_buffer_size, sizeof(native_type), ctx, mem_uncached_descr);

            mem_send.enqueue_write_sync(send_values);
            mem_recv.enqueue_write_sync(recv_values.begin(),
                                        recv_values.begin() + recv_buffer_size);
            temp_recv.enqueue_write_sync(recv_values.begin(),
                                         recv_values.begin() + 2 * recv_buffer_size);

            /* fill array in specific order
             * Left: l_send, l_recv, l_tmp_recv, r_tmp_recv
             * Right: r_send, r_recv, r_tmp_recv, l_tmp_recv
             */
            this->register_shared_memories_data(
                rank_device_idx, std::move(mem_send), std::move(mem_recv), std::move(temp_recv));

            // flags
            auto left_wrote_2_me_flag =
                sub_device->alloc_memory<int>(1, sizeof(int), ctx, mem_uncached_descr);
            auto ready_for_receive_flag =
                sub_device->alloc_memory<int>(1, sizeof(int), ctx, mem_uncached_descr);
            auto barrier_flag = sub_device->alloc_memory<int>(1, sizeof(int), ctx);
            left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
            ready_for_receive_flag.enqueue_write_sync({ (int)0 });
            barrier_flag.enqueue_write_sync({ (int)0 });

            /* fill array in specific order
             * Left: l_L, l_R, l_B, r_L, r_R
             * Right: r_L, r_R, r_B, l_L, L_R
             */
            this->register_shared_flags_data(rank_device_idx,
                                             std::move(left_wrote_2_me_flag),
                                             std::move(ready_for_receive_flag),
                                             std::move(barrier_flag));

            rank_device_idx++;
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false,
                      "Cannot allocate memory for thread: " << rank_device_idx
                                                            << "\nError: " << ex.what());
        }
    }
    this->finalize_data_registration(comm_group_count, mem_group_count, flag_group_count);

    // prepare kernels
    for (size_t device_index = 0; device_index < subdevices.size(); device_index++) {
        this->create_kernel(device_index,
                            reduce_scatter_param_traits<native_type, op_type>::kernel_name);
    }

    // prepare queues & lists
    std::map<size_t, ccl_device::device_queue> thread_queue;
    std::map<size_t, ccl_device::device_cmd_list> thread_cmd_list;
    multi_tile_utils::prepare_queues_and_lists(
        subdevices, ctx, thread_queue, thread_cmd_list, this->output);

    //printout memory handles
    this->dump_memory(this->output, true);

    // launch kernel for each device in separate thread
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    for (const auto& subdevice : subdevices) {
        size_t thread_idx = thread_group.size();
        ze_kernel_handle_t kernel = this->get_kernel(thread_idx);
        auto& mem_handles = this->get_memory_handles(thread_idx);
        auto& flag_handles = this->get_flag_handles(thread_idx);
        auto& comm_handles = this->get_comm_handles(thread_idx);

        this->output << "Launch kernel params: \n"
                     << " Sub_device idx" << ccl::to_string(subdevice->get_device_path())
                     << ",  Rank idx" << rank_device_idx << std::endl;

        ccl_device::device_queue& queue = thread_queue.find(thread_idx)->second;
        ccl_device::device_cmd_list& list = thread_cmd_list.find(thread_idx)->second;

        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();
        thread_group.emplace_back([this,
                                   thread_idx,
                                   kernel,
                                   &list,
                                   &queue,
                                   &mem_handles,
                                   &flag_handles,
                                   &comm_handles,
                                   raw_out]() {
            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { 1, 1, 1 };
            try {
                out << "Binding kernels arguments for thread:" << thread_idx << std::endl;
                // bind rank, size, send_elem_count
                out << "thread_idx: " << thread_idx << " - "
                    << "comm_offset" << std::endl;
                std::array<int, 3> comm_offset{ 0, 1, 2 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                bind_kernel_args(kernel, thread_idx, comm_offset, comm_handles);

                // bind l_send, l_recv, l_tmp, , , r_tmp
                out << "thread_idx: " << thread_idx << " - "
                    << "mem_offset" << std::endl;
                std::array<int, mem_group_count * 2> mem_offset{ 3, 4, 5, -1, 9, 10 };
                bind_kernel_args(kernel, thread_idx, mem_offset, mem_handles);

                // bind left_wrote_2_me_flag, ready_for_receive_flag, local_barrier_flag
                out << "thread_idx: " << thread_idx << " - "
                    << "flag_offset" << std::endl;
                std::array<int, flag_group_count * 2> flag_offset{ 6, 7, 8, 11, 12, -1 };
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
        thread_out_put.push_back(std::move(out_ptr));
    }

    size_t index = 0;
    for (auto& t : thread_group) {
        t.join();
        this->output << thread_out_put[index]->str() << std::endl;
        index++;
    }

    // printout result
    this->dump_memory(this->output);
}
} // namespace ring_single_device_multi_tile_case
