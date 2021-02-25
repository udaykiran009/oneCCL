#pragma once
#include <memory>
#include <sstream>

#define HOST_CTX
#include "kernels/a2a_helpers.h"

/**
 * Add custom types support into native::memory example
 */
/* 1) Describe new type traits */
#include "native_type_traits.hpp"

/* 2) Include explicit definition for native::memory */
#include "oneapi/ccl/native_device_api/l0/primitives_impl.hpp"

/* 3) just use it! */
#include "bcast_fixture.hpp"

namespace a2a_single_device_case {

namespace a2a_bcast_case {

template <class T>
struct a2a_param_traits;

#define DEFINE_KERNEL_TYPE_A2A(NAME, T) \
    template <> \
    struct a2a_param_traits<T> { \
        static constexpr const char* kernel_name = "bcast_execution_" #NAME; \
        using comm_data_type = a2a_gpu_comm_data_##NAME; \
    };

DEFINE_KERNEL_TYPE_A2A(int8, int8_t)
DEFINE_KERNEL_TYPE_A2A(uint8, uint8_t)
DEFINE_KERNEL_TYPE_A2A(int16, int16_t)
DEFINE_KERNEL_TYPE_A2A(uint16, uint16_t)
DEFINE_KERNEL_TYPE_A2A(int32, int32_t)
DEFINE_KERNEL_TYPE_A2A(uint32, uint32_t)
DEFINE_KERNEL_TYPE_A2A(int64, int64_t)
DEFINE_KERNEL_TYPE_A2A(uint64, uint64_t)
/*DEFINE_KERNEL_TYPE_A2A(float16, ushort)*/
DEFINE_KERNEL_TYPE_A2A(float32, float)
DEFINE_KERNEL_TYPE_A2A(float64, double)
// DEFINE_KERNEL_TYPE_A2A(bfloat16, ushort)

} // namespace a2a_bcast_case

TYPED_TEST_CASE(a2a_bcast_single_process_fixture, TestTypes);

TYPED_TEST(a2a_bcast_single_process_fixture, a2a_bcast_single_device_mt) {
    using namespace native;
    // Type of our test
    using native_type = TypeParam;
    std::shared_ptr<ccl_context> ctx;

    // test case data
    const size_t buffer_size = 512;
    const int comm_size = 4;
    constexpr size_t mem_group_count = 2;
    constexpr size_t a2a_mem_group_count = 1;
    constexpr size_t flag_group_count = 3;

    this->create_module_descr("kernels/a2a_bcast.spv", true);

    handles_storage<native_type> memory_storage(42 * comm_size);
    handles_storage<int> flags_storage(42 * comm_size);
    std::map<int, std::vector<int>> comm_param_storage;

    // check global driver
    auto drv_it = this->local_platform->drivers.find(0);
    UT_ASSERT(drv_it != this->local_platform->drivers.end(), "Driver by idx 0 must exist!");
    ccl_device_driver& driver = *drv_it->second;

    // check devices per process
    UT_ASSERT(driver.devices.size() > 0,
              "SingleDevice test scope require at least 1 device in local platform! Use correct \""
                  << ut_device_affinity_mask_name << "\"");

    std::vector<size_t> thread_indices;

    // device memory stencil data
    std::vector<native_type> send_values(buffer_size);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<native_type> recv_values(buffer_size, 0);

    // allocate device memory
    auto dev_it = driver.devices.begin();
    ccl_device& device = *dev_it->second;
    int root = 2;

    for (int rank = 0; rank < comm_size; rank++) {
        thread_indices.push_back(rank);
        try {
            //initialize communication params
            comm_param_storage[rank].push_back(rank);
            comm_param_storage[rank].push_back(comm_size);
            comm_param_storage[rank].push_back(buffer_size);
            comm_param_storage[rank].push_back(root);

            //allocate flags & memory
            // memory
            auto mem_send = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
            auto mem_recv = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
            //auto temp_recv = device.alloc_memory<native_type>(buffer_size, sizeof(native_type));

            if (rank == root) {
                mem_send.enqueue_write_sync(send_values);
            }
            else {
                mem_send.enqueue_write_sync(recv_values);
            }

            mem_recv.enqueue_write_sync(recv_values);
            //temp_recv.enqueue_write_sync(recv_values);

            /* fill array in specific order
             * Left: l_send, l_recv, l_tmp_recv, r_tmp_recv
             * Right: r_send, r_recv, r_tmp_recv, l_tmp_recv
             */
            memory_storage.register_shared_data(
                rank, comm_size, std::move(mem_send), std::move(mem_recv));

            // flags
            auto left_wrote_2_me_flag = device.alloc_memory<int>(1, sizeof(int), ctx);
            auto read_for_receive_flag = device.alloc_memory<int>(1, sizeof(int), ctx);
            auto barrier_flag = device.alloc_memory<int>(1, sizeof(int), ctx);
            left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
            read_for_receive_flag.enqueue_write_sync({ (int)0 });
            barrier_flag.enqueue_write_sync({ (int)0 });

            /* fill array in specific order
             * Left: l_L, l_R, l_B, r_L, r_R
             * Right: r_L, r_R, r_B, l_L, L_R
             */
            flags_storage.register_shared_data(rank,
                                               comm_size,
                                               std::move(left_wrote_2_me_flag),
                                               std::move(read_for_receive_flag),
                                               std::move(barrier_flag));
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false,
                      "Cannot allocate memory for rank: " << rank << "\nError: " << ex.what());
        }
    }

    for (size_t rank = 0; rank < comm_size; rank++) {
        memory_storage.rotate_shared_data(rank, comm_size, mem_group_count);
        flags_storage.rotate_shared_data(rank, comm_size, flag_group_count);
    }

    //get handles for A2A
    typename handles_storage<native_type>::rank_handles_container rank_mem =
        memory_storage.collect_handles_by_index({ 1 });
    typename handles_storage<int>::rank_handles_container rank_flags =
        flags_storage.collect_handles_by_index({ 0, 1 });

    std::vector<typename a2a_bcast_case::a2a_param_traits<native_type>::comm_data_type> a2a_comm(
        comm_size);

    //Register memory handles to A2A
    for (size_t thread_id = 0; thread_id < rank_mem.size(); thread_id++) {
        a2a_comm[thread_id].recv_buf = *rank_mem[thread_id].begin();
    }
    //Register flag handles to A2A
    for (size_t thread_id = 0; thread_id < rank_flags.size(); thread_id++) {
        a2a_comm[thread_id].data_sent_flag = *rank_flags[thread_id].begin();
        a2a_comm[thread_id].ready_to_receive_flag = *std::next(rank_flags[thread_id].begin());
    }

    //prepare gpu object
    auto a2a_comm_handle =
        device.alloc_memory<typename a2a_bcast_case::a2a_param_traits<native_type>::comm_data_type>(
            comm_size,
            sizeof(typename a2a_bcast_case::a2a_param_traits<native_type>::comm_data_type),
            ctx);
    a2a_comm_handle.enqueue_write_sync(a2a_comm);

    //prepare kernels in multithreading environment
    ze_kernel_desc_t desc = {
        .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
        .pNext = nullptr,
        .flags = 0,
    };
    desc.pKernelName = a2a_bcast_case::a2a_param_traits<native_type>::kernel_name;
    std::map<size_t, ze_kernel_handle_t> rank_kernels;
    std::map<size_t, ccl_device::device_queue> rank_queues;
    std::map<size_t, ccl_device::device_cmd_list> rank_cmd_lists;
    ccl_device::device_module& module = *(this->device_modules.find(&device)->second);
    for (int rank = 0; rank < comm_size; rank++) {
        //thread_group.emplace
        ze_kernel_handle_t handle = nullptr;
        try {
            ze_result_t result = zeKernelCreate(module.handle, &desc, &handle);
            if (result != ZE_RESULT_SUCCESS) {
                throw std::runtime_error(std::string("Cannot create kernel: ") + desc.pKernelName +
                                         ", error: " + native::to_string(result));
            }

            rank_kernels.emplace(rank, std::move(handle));
            rank_queues.emplace(rank, device.create_cmd_queue(ctx));
            rank_cmd_lists.emplace(rank, device.create_cmd_list(ctx));
        }
        catch (const std::exception& ex) {
            throw std::runtime_error(std::string("Error: ") + ex.what());
        }
    }

    auto& out = this->output;
    //printout
    // out << "L0 memory handles: " << std::endl;
    // memory_storage.dump(out, true);
    // memory_storage.dump_by_index(out, 0 /*secv_mem*/);

    //Set args and launch kernel
    std::mutex thread_lock; //workaround
    std::atomic<size_t> val{ 0 }; //workaround
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    for (auto& idx_kernel : rank_kernels) {
        size_t rank = idx_kernel.first;
        ze_kernel_handle_t kernel = idx_kernel.second;
        auto& mem_handles = find_storage_val(memory_storage.per_rank_storage, rank);
        auto& flag_handles = find_storage_val(flags_storage.per_rank_storage, rank);
        auto& comm_handles = find_storage_val(comm_param_storage, rank);
        (void)flag_handles;

        //WORKAROUND: ONLY ONE LIST & QUEUE!
        //ccl_device::device_queue& queue = rank_queues.find(rank)->second;
        ccl_device::device_queue& queue = rank_queues.find(0)->second;
        //ccl_device::device_cmd_list& list = rank_cmd_lists.find(rank)->second;
        ccl_device::device_cmd_list& list = rank_cmd_lists.find(0)->second;

        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();
        thread_group.emplace_back([this,
                                   &a2a_comm_handle,
                                   rank,
                                   kernel,
                                   &list,
                                   &queue,
                                   &mem_handles,
                                   &comm_handles,
                                   &thread_lock,
                                   &val,
                                   raw_out]() {
            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { comm_size, 1, 1 };
            try {
                ze_result_t result;
                out << "rank: " << rank << ", comm_handles: \n";

                // bind rank, size, buffer_size
                size_t i = 0;
                std::array<int, 4> comm_offset{ 0, 1, 2, 5 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                for (auto& comm : comm_handles) {
                    out << "index: " << comm_offset[i] << ": " << comm << std::endl;
                    result = zeKernelSetArgumentValue(kernel, comm_offset[i], sizeof(comm), &comm);
                    if (result != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("Cannot zeKernelSetArgumentValue memory at comm_offset: ") +
                            std::to_string(comm_offset[i]) +
                            " index\nError: " + native::to_string(result));
                    }
                    i++;
                }
                out << std::endl;

                // bind l_send, l_recv, l_tmp, , , r_tmp
                i = 0;
                std::array<int, a2a_mem_group_count> mem_offset{ 3 };
                //UT_ASSERT(mem_offset.size() == mem_handles.size(), "mem_offset != mem_handles");
                out << "rank: " << rank << ", mem_handles: \n";
                for (auto& mem : mem_handles) {
                    if (i >= a2a_mem_group_count) {
                        break; //only own is needed
                    }
                    out << "index: " << mem_offset[i] << ": " << (void*)mem << std::endl;
                    result = zeKernelSetArgumentValue(kernel, mem_offset[i], sizeof(mem), &mem);
                    if (result != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("Cannot zeKernelSetArgumentValue memory at mem_offset: ") +
                            std::to_string(mem_offset[i]) +
                            " index\nError: " + native::to_string(result));
                    }
                    i++;
                }
                out << std::endl;

                {
                    int a2a_comm_offset = 4;
                    out << "index: " << a2a_comm_offset << ": " << a2a_comm_handle.handle
                        << std::endl;
                    result = zeKernelSetArgumentValue(kernel,
                                                      a2a_comm_offset,
                                                      sizeof(a2a_comm_handle.handle),
                                                      &a2a_comm_handle.handle);
                    if (result != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string(
                                "Cannot zeKernelSetArgumentValue a2a comm at a2a_comm_offset: ") +
                            std::to_string(a2a_comm_offset) +
                            " index\nError: " + native::to_string(result));
                    }
                }
                out << std::endl;

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
                    ret = zeCommandListClose(list.handle);
                    if (ret != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(std::string("cannot zeCommandListClose, error: ") +
                                                 std::to_string(ret));
                    }

                    ret = zeCommandQueueExecuteCommandLists(queue.handle, 1, &list.handle, nullptr);
                    if (ret != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("cannot zeCommandQueueExecuteCommandLists, error: ") +
                            std::to_string(ret));
                    }

                    ret = zeCommandQueueSynchronize(queue.handle,
                                                    std::numeric_limits<uint32_t>::max());
                    if (ret != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("cannot zeCommandQueueSynchronize, error: ") +
                            std::to_string(ret));
                    }

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
        out << "Kernels argument binding log for rank: " << index << std::endl;
        out << thread_out_put[index]->str() << std::endl;
        index++;
    }

    size_t corr_val = 0;
    try {
        for (auto& idx_kernel : rank_kernels) {
            size_t rank = idx_kernel.first;
            auto lambda =
                [&corr_val](
                    const int root, size_t rank, size_t buffer_size, native_type value) -> bool {
                corr_val++;
                if (corr_val > buffer_size)
                    corr_val = 1;
                if (value != static_cast<native_type>(corr_val)) {
                    return false;
                }
                return true;
            };

            memory_storage.check_results(rank, out, 1, lambda, root, rank, buffer_size);
        }
    }
    catch (check_on_exception& ex) {
        out << "Check results: \n";
        //printout
        out << "Send memory:" << std::endl;
        memory_storage.dump_by_index(out, 0 /*send_mem*/);
        out << "\nRecv memory:" << std::endl;
        memory_storage.dump_by_index(out, 1 /*recv_mem*/);

        std::stringstream ss;
        ss << ex.what() << ", But expected: " << corr_val << std::endl;
        UT_ASSERT(false, ss.str());
    }
}

} // namespace a2a_single_device_case
