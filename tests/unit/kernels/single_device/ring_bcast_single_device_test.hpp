#pragma once

#include <memory>
#include <sstream>
#include <cstdint>

#include "bcast_fixture.hpp"
#include "single_device_utils.hpp"

DEFINE_KERNEL_TYPES(bcast)

namespace ring_single_device_case {

namespace ring_bcast_case {}

TYPED_TEST_CASE(ring_bcast_single_process_fixture, TestTypes);
TYPED_TEST(ring_bcast_single_process_fixture, ring_bcast_single_device_mt) {
    using namespace native;

    // Type of our test
    using native_type = TypeParam;

    // check test preconditions
    const auto& devices = this->get_local_devices();
    UT_ASSERT(devices.size() > 0,
              "SingleDevice test scope require at least 1 device in local platform! Use correct \""
                  << ut_device_affinity_mask_name << "\"");

    UT_ASSERT(this->get_unique_local_devices().size() == 1,
              "Devices must not be unique to launch single device case");

    // declare test case data
    const size_t buffer_size = 512;
    const size_t num_thread = devices.size();
    constexpr size_t comm_group_count = 4;
    constexpr size_t mem_group_count = 1;
    constexpr size_t flag_group_count = 3;

    // allocate & initialize test kernel params
    std::shared_ptr<ccl_context> ctx;
    std::vector<native_type> send_values(buffer_size);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<native_type> recv_values(buffer_size, 0);
    int root = 1;

    // allocate & initialize test kernel params
    for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        //initialize communication params
        int rank_idx = thread_idx;
        int rank_size = num_thread;
        size_t elem_count = buffer_size;
        this->output << "Create device memory & flags handles for device by index: "
                     << std::to_string(devices[thread_idx]->get_device_id()) << ", as rank: ("
                     << rank_idx << "/" << rank_size << ")" << std::endl;

        this->register_shared_comm_data(rank_idx, rank_idx, rank_size, elem_count, root);

        //allocate flags & memory
        // memory
        auto mem_buf = devices[thread_idx]->template alloc_memory<native_type>(
            buffer_size, sizeof(native_type), ctx);

        if (rank_idx == root) {
            mem_buf.enqueue_write_sync(send_values);
        }
        else {
            mem_buf.enqueue_write_sync(recv_values);
        }

        this->register_shared_memories_data(rank_idx, std::move(mem_buf));

        // flags
        this->output << "Alloc flag handles: " << std::endl;
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
        this->create_kernel(device_index, bcast_param_traits<native_type>::kernel_name);
    }

    // prepare queues & lists
    std::map<size_t, ccl_device::device_queue> thread_queue;
    std::map<size_t, ccl_device::device_cmd_list> thread_cmd_list;

    single_device_utils::prepare_kernel_queues_lists(
        devices, ctx, thread_queue, thread_cmd_list, this->output);

    //printout memory handles
    this->dump_memory(this->output, true);

    std::mutex thread_lock; //workaround
    std::atomic<size_t> val{ 0 }; //workaround
    // launch kernel for each device in separate thread
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
                // bind rank, size, buffer_size
                out << "thread_idx: " << thread_idx << " - "
                    << "comm_offset" << std::endl;
                std::array<int, 4> comm_offset{ 0, 1, 2, 10 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                bind_kernel_args(kernel, thread_idx, comm_offset, comm_handles);

                // bind l_send, l_recv, l_tmp, , , r_tmp
                // bind l_send, l_recv, , , r_recv, ,
                out << "thread_idx: " << thread_idx << " - "
                    << "mem_offset" << std::endl;
                std::array<int, mem_group_count * 2> mem_offset{ 3, 7 };
                bind_kernel_args(kernel, thread_idx, mem_offset, mem_handles);

                // bind left_wrote_2_me_flag, read_for_receive_flag, local_barrier_flag
                out << "thread_idx: " << thread_idx << " - "
                    << "flag_offset" << std::endl;
                std::array<int, flag_group_count * 2> flag_offset{ 4, 5, 6, 8, 9, -1 };
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

    size_t corr_val = 0;
    try {
        for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
            auto lambda = [&corr_val](const int root,
                                      size_t thread_idx,
                                      size_t buffer_size,
                                      native_type value) -> bool {
                corr_val++;
                if (corr_val > buffer_size)
                    corr_val = 1;
                if (value != static_cast<native_type>(corr_val))
                    return false;
                return true;
            };

            this->check_test_results(
                thread_idx, this->output, 0 /*recv_mem*/, lambda, root, thread_idx, buffer_size);
        }
    }
    catch (check_on_exception& ex) {
        this->output << "Check results: \n";
        //printout
        this->output << "Send memory:" << std::endl;
        this->dump_memory_by_index(this->output, 0 /*send_mem*/);
        this->output << "\nRecv memory:" << std::endl;
        this->dump_memory_by_index(this->output, 1 /*recv_mem*/);

        std::stringstream ss;
        ss << ex.what() << ", But expected: " << corr_val * num_thread << std::endl;
        UT_ASSERT(false, ss.str());
    }
}
} // namespace ring_single_device_case
