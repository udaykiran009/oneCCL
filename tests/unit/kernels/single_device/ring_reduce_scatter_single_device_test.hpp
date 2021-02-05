#pragma once

#include <memory>
#include <sstream>

#include "reduce_scatter_fixture.hpp"
#include "single_device_utils.hpp"

DEFINE_KERNEL_TYPES_FOR_OP_BF16(reduce_scatter, sum);
DEFINE_KERNEL_TYPES_FOR_OP_BF16(reduce_scatter, prod);
DEFINE_KERNEL_TYPES_FOR_OP_BF16(reduce_scatter, min);
DEFINE_KERNEL_TYPES_FOR_OP_BF16(reduce_scatter, max);

namespace ring_single_device_case {

namespace ring_reduce_scatter_case {}

TYPED_TEST_CASE(ring_reduce_scatter_single_process_fixture, TestTypesAndOpsReduction);

TYPED_TEST(ring_reduce_scatter_single_process_fixture, ring_reduce_scatter_single_device_mt) {
    using namespace native;

    // Type of our test
    using native_type = typename TypeParam::first_type;
    using op_type = typename TypeParam::second_type;

    // check test preconditions
    const auto& devices = this->get_local_devices();
    UT_ASSERT(devices.size() > 0,
              "SingleDevice test scope require at least 1 device in local platform! Use correct \""
                  << ut_device_affinity_mask_name << "\"");

    UT_ASSERT(this->get_unique_local_devices().size() == 1,
              "Devices must not be unique to launch single device case");

    // test case data
    const size_t num_thread = devices.size();
    const size_t recv_buffer_size = 64;
    const size_t send_buffer_size = num_thread * recv_buffer_size;
    constexpr size_t comm_group_count = 3;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;

    // fill device memory stencil data
    std::shared_ptr<ccl_context> ctx;
    std::map<size_t, std::vector<native_type>> send_values;
    std::map<size_t, std::vector<native_type>> recv_values;
    std::map<size_t, std::vector<native_type>> calc_values;
    for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        size_t mult = 0;
        for (size_t idx = 1; idx <= send_buffer_size; ++idx, ++mult) {
            send_values[thread_idx].push_back(
                static_cast<native_type>(idx * ((thread_idx + mult) % num_thread + 1)));
            calc_values[thread_idx].push_back(static_cast<native_type>(0));
        }
        for (size_t idx = 1; idx <= send_buffer_size; ++idx, ++mult) {
            recv_values[thread_idx].push_back(static_cast<native_type>(0));
        }
    }

    for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        //initialize communication params
        int rank_idx = thread_idx;
        int rank_size = num_thread;
        size_t send_elem_count = send_buffer_size;
        this->output << "Device id" << ccl::to_string(devices[thread_idx]->get_device_path())
                     << ", rank id" << rank_idx << std::endl;

        this->register_shared_comm_data(rank_idx, rank_idx, rank_size, send_elem_count);

        //allocate flags & memory
        // memory
        auto mem_send = devices[thread_idx]->template alloc_memory<native_type>(
            send_buffer_size, sizeof(native_type), ctx);
        auto mem_recv = devices[thread_idx]->template alloc_memory<native_type>(
            recv_buffer_size, sizeof(native_type), ctx);
        auto temp_recv = devices[thread_idx]->template alloc_memory<native_type>(
            2 * recv_buffer_size, sizeof(native_type), ctx);

        mem_send.enqueue_write_sync(send_values[thread_idx]);
        mem_recv.enqueue_write_sync(recv_values[thread_idx].begin(),
                                    recv_values[thread_idx].begin() + recv_buffer_size);
        temp_recv.enqueue_write_sync(recv_values[thread_idx].begin(),
                                     recv_values[thread_idx].begin() + 2 * recv_buffer_size);

        /* fill array in specific order
         * Left: l_send, l_recv, l_tmp_recv, r_tmp_recv
         * Right: r_send, r_recv, r_tmp_recv, l_tmp_recv
         */
        this->register_shared_memories_data(
            rank_idx, std::move(mem_send), std::move(mem_recv), std::move(temp_recv));

        // flags
        auto left_wrote_2_me_flag =
            devices[thread_idx]->template alloc_memory<int>(1, sizeof(int), ctx);
        auto ready_for_receive_flag =
            devices[thread_idx]->template alloc_memory<int>(1, sizeof(int), ctx);
        auto barrier_flag = devices[thread_idx]->template alloc_memory<int>(1, sizeof(int), ctx);
        left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
        ready_for_receive_flag.enqueue_write_sync({ (int)0 });
        barrier_flag.enqueue_write_sync({ (int)0 });

        /* fill array in specific order
         * Left: l_L, l_R, l_B, r_L, r_R
         * Right: r_L, r_R, r_B, l_L, L_R
         */
        this->register_shared_flags_data(rank_idx,
                                         std::move(left_wrote_2_me_flag),
                                         std::move(ready_for_receive_flag),
                                         std::move(barrier_flag));
    }

    this->finalize_data_registration(comm_group_count, mem_group_count, flag_group_count);

    // prepare kernels
    for (size_t device_index = 0; device_index < num_thread; device_index++) {
        this->create_kernel(device_index,
                            reduce_scatter_param_traits<native_type, op_type>::kernel_name);
    }

    // prepare queues & lists
    std::map<size_t, ccl_device::device_queue> thread_queue;
    std::map<size_t, ccl_device::device_cmd_list> thread_cmd_list;

    single_device_utils::prepare_kernel_queues_lists(
        devices, ctx, thread_queue, thread_cmd_list, this->output);

    //printout memory handles
    this->dump_memory(this->output, true);

    //Set args and launch kernel
    std::mutex thread_lock; //workaround
    std::atomic<size_t> val{ 0 }; //workaround
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        ze_kernel_handle_t kernel = this->get_kernel(thread_idx);
        auto& mem_handles = this->get_memory_handles(thread_idx);
        auto& flag_handles = this->get_flag_handles(thread_idx);
        auto& comm_handles = this->get_comm_handles(thread_idx);

        this->output << "Launch kernel params: \n"
                     << " Device idx" << ccl::to_string(devices[thread_idx]->get_device_path())
                     << ",  Rank idx" << thread_idx << std::endl;

        //WORKAROUND: ONLY ONE LIST & QUEUE!
        ccl_device::device_queue& queue = thread_queue.find(0)->second;
        ccl_device::device_cmd_list& list = thread_cmd_list.find(0)->second;

        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();
        thread_group.emplace_back([this,
                                   thread_idx,
                                   kernel,
                                   num_thread,
                                   &list,
                                   &queue,
                                   &mem_handles,
                                   &flag_handles,
                                   &comm_handles,
                                   &thread_lock,
                                   &val,
                                   raw_out]() {
            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { 1, 1, 1 };
            try {
                out << "Binding kernels arguments for thread:" << thread_idx << std::endl;
                // bind rank, size, send_elem_count
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

    // waiting for threads with kernel execution
    size_t index = 0;
    for (auto& t : thread_group) {
        t.join();
        this->output << thread_out_put[index]->str() << std::endl;
        index++;
    }

    // Copy device buffers back to host...
    for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        auto& mem_handles = this->get_memory_handles(thread_idx);
        size_t mem_idx = 1; // Recv memory
        size_t i = 0;
        for (auto iter_mem = mem_handles.begin(); iter_mem != mem_handles.end(); iter_mem++, i++) {
            if (i != mem_idx) {
                continue;
            }
            const auto* data = *iter_mem;
            auto it =
                std::find_if(this->get_memory_storage().allocated_storage.begin(),
                             this->get_memory_storage().allocated_storage.end(),
                             [data](const native::ccl_device::device_memory<native_type>& wrapper) {
                                 return wrapper.handle == data;
                             });

            recv_values[thread_idx] = it->enqueue_read_sync();
        }
    }

    // Compute output buffers in the same order as within the OCl kernel i.e. do ring reduce-scatter on host...
    size_t segment_offset_idx, right_thread_idx, buffer_offset;
    size_t segment_size = send_buffer_size / num_thread;
    native_type nbr_send_value, send_value, temp_calc_value;
    constexpr auto op = op_type{};
    // For each thread, populate the calc_values of the right neighbor
    for (size_t thread_idx = 0; thread_idx < num_thread; ++thread_idx) {
        right_thread_idx = (thread_idx + 1) % num_thread;
        for (size_t segment_idx = 0; segment_idx < send_buffer_size; ++segment_idx) {
            calc_values[right_thread_idx][segment_idx] = send_values[thread_idx][segment_idx];
        }
    }
    // Now do ring reduce-scatter
    for (size_t phase_idx = 1; phase_idx < num_thread; ++phase_idx) {
        for (size_t thread_idx = 0; thread_idx < num_thread; ++thread_idx) {
            right_thread_idx = (thread_idx + 1) % num_thread;
            segment_offset_idx = (thread_idx - phase_idx + num_thread) % num_thread;
            buffer_offset = segment_offset_idx * segment_size;
            for (size_t segment_idx = 0; segment_idx < segment_size; ++segment_idx) {
                nbr_send_value = calc_values[thread_idx][buffer_offset + segment_idx];
                send_value = send_values[thread_idx][buffer_offset + segment_idx];
                temp_calc_value = op(nbr_send_value, send_value);
                calc_values[right_thread_idx][buffer_offset + segment_idx] = temp_calc_value;
            }
        }
    }

    // Check results against host...
    try {
        size_t segment_offset;
        for (size_t thread_idx = 0; thread_idx < num_thread; ++thread_idx) {
            segment_offset = thread_idx * segment_size;
            for (size_t segment_idx = 0; segment_idx < segment_size; ++segment_idx) {
                if (recv_values[thread_idx][segment_idx] !=
                    calc_values[thread_idx][segment_offset + segment_idx]) {
                    throw comparison_exception(
                        to_string(thread_idx),
                        to_string(segment_idx),
                        to_string(recv_values[thread_idx][segment_idx]),
                        to_string(calc_values[thread_idx][segment_offset + segment_idx]));
                }
            }
        }
    }
    catch (comparison_exception& ex) {
        this->output << "Check results: \n";
        //printout
        this->output << "Send memory:" << std::endl;
        this->dump_memory_by_index(this->output, 0 /*send_mem*/);
        this->output << std::endl;
        this->output << "Recv memory:" << std::endl;
        this->dump_memory_by_index(this->output, 1 /*recv_mem*/);
        this->output << std::endl;
        this->output << "Exp memory: " << std::endl;

        size_t segment_size = send_buffer_size / num_thread, segment_offset;
        for (size_t thread_idx = 0; thread_idx < num_thread; ++thread_idx) {
            this->output << "Thread: " << thread_idx << ", handle: " << std::endl;
            segment_offset = segment_size * thread_idx;
            for (size_t segment_idx = 0; segment_idx < segment_size; ++segment_idx) {
                this->output << segment_offset + segment_idx << ": "
                             << calc_values[thread_idx][segment_offset + segment_idx] << "; ";
            }
            this->output << std::endl << std::endl << std::endl;
        }
        this->output << std::endl;

        std::stringstream ss;
        ss << ex.what() << std::endl;
        UT_ASSERT(false, ss.str());
    }
}

} // namespace ring_single_device_case
