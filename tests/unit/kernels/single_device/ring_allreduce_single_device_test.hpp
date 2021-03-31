#pragma once

#include <memory>
#include <cstdint>
#include <sstream>

#include "allreduce_fixture.hpp"
#include "single_device_utils.hpp"

DEFINE_KERNEL_TYPES_FOR_OP(allreduce, sum);
DEFINE_KERNEL_TYPES_FOR_OP(allreduce, prod);
DEFINE_KERNEL_TYPES_FOR_OP(allreduce, min);
DEFINE_KERNEL_TYPES_FOR_OP(allreduce, max);

namespace ring_single_device_case {

namespace ring_allreduce_case {}

TYPED_TEST_CASE(ring_allreduce_single_process_fixture, TestTypesAndOps);

TYPED_TEST(ring_allreduce_single_process_fixture, ring_allreduce_single_device_mt) {
    using namespace native;

    std::shared_ptr<ccl_context> ctx;

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
    const size_t buffer_size = get_from_env_or("UT_KERNEL_BUF_SIZE", 512 * 4);
    const size_t comm_size = devices.size();
    constexpr size_t comm_group_count = 3;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;

    alloc_and_fill_allreduce_buffers<native_type>(this, buffer_size, devices, ctx);

    for (size_t rank = 0; rank < comm_size; rank++) {
        //initialize communication params
        this->output << "device id: " << ccl::to_string(devices[rank]->get_device_path())
                     << ", rank: " << rank << std::endl;

        this->register_shared_comm_data(rank, rank, comm_size, buffer_size);

        // allocate flags & memory
        ze_device_mem_alloc_desc_t mem_uncached_descr{
            .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
            .pNext = NULL,
            .flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED,
            .ordinal = 0,
        };

        // flags
        auto left_wrote_2_me_flag =
            devices[rank]->template alloc_memory<int>(1, sizeof(int), ctx, mem_uncached_descr);
        auto ready_for_receive_flag =
            devices[rank]->template alloc_memory<int>(1, sizeof(int), ctx, mem_uncached_descr);
        auto barrier_flag = devices[rank]->template alloc_memory<int>(1, sizeof(int), ctx);
        left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
        ready_for_receive_flag.enqueue_write_sync({ (int)0 });
        barrier_flag.enqueue_write_sync({ (int)0 });

        /* fill array in specific order
             * Left: l_L, l_R, l_B, r_L, r_R
             * Right: r_L, r_R, r_B, l_L, L_R
             */
        this->register_shared_flags_data(rank,
                                         std::move(left_wrote_2_me_flag),
                                         std::move(ready_for_receive_flag),
                                         std::move(barrier_flag));
    }

    this->finalize_data_registration(comm_group_count, mem_group_count, flag_group_count);

    // prepare kernels
    for (size_t device_index = 0; device_index < comm_size; device_index++) {
        this->create_kernel(device_index,
                            allreduce_param_traits<native_type, op_type>::kernel_name);
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

        this->output << "Launch kernel params: \n"
                     << " device id: " << ccl::to_string(devices[rank]->get_device_path())
                     << ", rank: " << rank << std::endl;

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
                                   &thread_lock,
                                   &val,
                                   raw_out]() {
            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { 1, 1, 1 };
            try {
                out << "Binding kernels arguments for rank: " << rank << std::endl;
                // bind rank, size, buffer_size
                out << "rank: " << rank << " - "
                    << "comm_offset" << std::endl;
                std::array<int, 3> comm_offset{ 0, 1, 2 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                bind_kernel_args(kernel, rank, comm_offset, comm_handles);

                // bind l_send, l_recv, l_tmp, , , r_tmp
                out << "rank: " << rank << " - "
                    << "mem_offset" << std::endl;
                std::array<int, mem_group_count * 2> mem_offset{ 3, 4, 5, -1, -1, 9 };
                bind_kernel_args(kernel, rank, mem_offset, mem_handles);

                // bind left_wrote_2_me_flag, ready_for_receive_flag, local_barrier_flag
                out << "rank: " << rank << " - "
                    << "flag_offset" << std::endl;
                std::array<int, flag_group_count * 2> flag_offset{ 6, 7, 8, 10, 11, -1 };
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

    // waiting for threads with kernel execution
    size_t index = 0;
    for (auto& t : thread_group) {
        t.join();
        this->output << thread_out_put[index]->str() << std::endl;
        index++;
    }

    check_allreduce_buffers<native_type, op_type>(this, buffer_size);
}

} // namespace ring_single_device_case
