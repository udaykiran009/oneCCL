#pragma once
#include <memory>
#include <sstream>

#define HOST_CTX
#include "kernels/a2a_helpers.h"

/**
 * Add custom types support into native::memory example
 */
/* 1) Describe new type traits */
namespace ccl {
template <>
struct native_type_info<a2a_gpu_comm_data_float> {
    static constexpr bool is_supported = true;
    static constexpr bool is_class = true;
};
} // namespace ccl
/* 2) Include explicit definition for native::memory */
#include "oneapi/ccl/native_device_api/l0/primitives_impl.hpp"

/* 3) just use it! */

namespace singledevice_case {

// test case data
static const size_t buffer_size = 512;
static const size_t num_thread = 4;

using native_type = float;

static constexpr size_t mem_group_count = 3;
static constexpr size_t a2a_mem_group_count = 2;
static constexpr size_t flag_group_count = 3;

TEST_F(allreduce_one_device_local_fixture, a2a_allreduce_one_device_multithread_kernel) {
    using namespace native;

    create_module_descr("kernels/a2a_allreduce.spv", true);

    handles_storage<native_type> memory_storage(42 * num_thread);
    handles_storage<int> flags_storage(42 * num_thread);
    std::map<size_t, std::vector<size_t>> comm_param_storage;

    // check global driver
    auto drv_it = local_platform->drivers.find(0);
    UT_ASSERT(drv_it != local_platform->drivers.end(), "Driver by idx 0 must exist!");
    ccl_device_driver& driver = *drv_it->second;

    // check devices per process
    UT_ASSERT(driver.devices.size() == local_affinity.size(),
              "Count: %" << driver.devices.size() << ", bits: " << local_affinity.size()
                         << "Device count is not equal to affinity mask!");

    std::vector<size_t> thread_indices;

    // device memory stencil data
    std::vector<native_type> send_values(buffer_size);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<native_type> recv_values(buffer_size, 0);

    // allocate device memory
    auto dev_it = driver.devices.begin();
    ccl_device& device = *dev_it->second;

    for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        thread_indices.push_back(thread_idx);
        try {
            //initialize communication params
            size_t rank_idx = thread_idx;
            size_t rank_size = num_thread;
            size_t elem_count = buffer_size;

            comm_param_storage[thread_idx].push_back(rank_idx);
            comm_param_storage[thread_idx].push_back(rank_size);
            comm_param_storage[thread_idx].push_back(elem_count);

            //allocate flags & memory
            // memory
            auto mem_send = device.alloc_memory<native_type>(buffer_size, sizeof(native_type));
            auto mem_recv = device.alloc_memory<native_type>(buffer_size, sizeof(native_type));
            auto temp_recv =
                device.alloc_memory<native_type>(buffer_size / num_thread, sizeof(native_type));

            mem_send.enqueue_write_sync(send_values);
            mem_recv.enqueue_write_sync(recv_values);
            temp_recv.enqueue_write_sync(recv_values.begin(),
                                         recv_values.begin() + buffer_size / num_thread);

            /* fill array in specific order
             * Left: l_send, l_recv, l_tmp_recv, r_tmp_recv
             * Right: r_send, r_recv, r_tmp_recv, l_tmp_recv
             */
            memory_storage.register_shared_data(thread_idx,
                                                num_thread,
                                                std::move(mem_send),
                                                std::move(mem_recv),
                                                std::move(temp_recv));

            // flags
            auto left_wrote_2_me_flag = device.alloc_memory<int>(1, sizeof(int));
            auto read_for_receive_flag = device.alloc_memory<int>(1, sizeof(int));
            auto barrier_flag = device.alloc_memory<int>(1, sizeof(int));
            left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
            read_for_receive_flag.enqueue_write_sync({ (int)0 });
            barrier_flag.enqueue_write_sync({ (int)0 });

            /* fill array in specific order
             * Left: l_L, l_R, l_B, r_L, r_R
             * Right: r_L, r_R, r_B, l_L, L_R
             */
            flags_storage.register_shared_data(thread_idx,
                                               num_thread,
                                               std::move(left_wrote_2_me_flag),
                                               std::move(read_for_receive_flag),
                                               std::move(barrier_flag));
        }
        catch (const std::exception& ex) {
            UT_ASSERT(
                false,
                "Cannot allocate memory for thread: %" << thread_idx << "\nError: " << ex.what());
        }
    }

    for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        memory_storage.rotate_shared_data(thread_idx, num_thread, mem_group_count);
        flags_storage.rotate_shared_data(thread_idx, num_thread, flag_group_count);
    }

    //get handles for A2A
    typename handles_storage<native_type>::thread_handles_container rank_mem =
        memory_storage.collect_handles_by_index({ 2 });
    typename handles_storage<int>::thread_handles_container rank_flags =
        flags_storage.collect_handles_by_index({ 0, 1 });

    std::unique_ptr<a2a_gpu_comm_data_float[]> a2a_comm(new a2a_gpu_comm_data_float[num_thread]);

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
        device.alloc_memory<a2a_gpu_comm_data_float>(num_thread, sizeof(a2a_gpu_comm_data_float));
    memcpy(a2a_comm_handle.handle, a2a_comm.get(), num_thread * sizeof(a2a_gpu_comm_data_float));

    //prepare kernels in multithreading environment
    ze_kernel_desc_t desc = { ZE_KERNEL_DESC_VERSION_CURRENT, ZE_KERNEL_FLAG_NONE };
    desc.pKernelName = "allreduce_execution_float";
    std::map<size_t, ze_kernel_handle_t> thread_kernels;
    std::map<size_t, ccl_device::device_queue> thread_queue;
    std::map<size_t, ccl_device::device_cmd_list> thread_cmd_list;
    ccl_device::device_module& module = *(device_modules.find(&device)->second);
    for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        //thread_group.emplace
        ze_kernel_handle_t handle = nullptr;
        try {
            ze_result_t result = zeKernelCreate(module.handle, &desc, &handle);
            if (result != ZE_RESULT_SUCCESS) {
                throw std::runtime_error(std::string("Cannot create kernel: ") + desc.pKernelName +
                                         ", error: " + native::to_string(result));
            }

            thread_kernels.emplace(thread_idx, std::move(handle));
            thread_queue.emplace(thread_idx, device.create_cmd_queue());
            thread_cmd_list.emplace(thread_idx, device.create_cmd_list());
        }
        catch (const std::exception& ex) {
            throw std::runtime_error(std::string("Error: ") + ex.what());
        }
    }

    //printout
    output << "L0 memory handles: " << std::endl;
    memory_storage.dump(output, true);
    memory_storage.dump_by_index(output, 0 /*secv_mem*/);

    //Set args and launch kernel
    std::mutex thread_lock; //workaround
    size_t val = 0; //workaround
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    for (auto& idx_kernel : thread_kernels) {
        size_t thread_idx = idx_kernel.first;
        ze_kernel_handle_t kernel = idx_kernel.second;
        auto& mem_handles = memory_storage.per_thread_storage.find(thread_idx)->second;
        auto& flag_handles = flags_storage.per_thread_storage.find(thread_idx)->second;
        auto& comm_handles = comm_param_storage.find(thread_idx)->second;
        (void)flag_handles;

        //WORKAROUND: ONLY ONE LIST & QUEUE!
        //ccl_device::device_queue& queue = thread_queue.find(thread_idx)->second;
        ccl_device::device_queue& queue = thread_queue.find(0)->second;
        //ccl_device::device_cmd_list& list = thread_cmd_list.find(thread_idx)->second;
        ccl_device::device_cmd_list& list = thread_cmd_list.find(0)->second;

        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();
        thread_group.emplace_back([this,
                                   &a2a_comm_handle,
                                   thread_idx,
                                   kernel,
                                   &list,
                                   &queue,
                                   &mem_handles,
                                   &comm_handles,
                                   &thread_lock,
                                   &val,
                                   raw_out]() {
            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { num_thread, 1, 1 };
            try {
                ze_result_t result;
                out << "thread_idx: " << thread_idx << ", comm_handles: \n";

                // bind rank, size, buffer_size
                size_t i = 0;
                std::array<int, 3> comm_offset{ 0, 1, 2 };
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
                std::array<int, a2a_mem_group_count> mem_offset{ 3, 4 };
                //UT_ASSERT(mem_offset.size() == mem_handles.size(), "mem_offset != mem_handles");
                out << "thread_idx: " << thread_idx << ", mem_handles: \n";
                for (auto& mem : mem_handles) {
                    if (i >= a2a_mem_group_count) {
                        break; //only own is needed
                    }
                    out << "index: " << mem_offset[i] << ": " << mem << std::endl;
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
                    int a2a_comm_offset = 5;
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

                // lock before submitting to the command list
                thread_lock.lock();

                ze_result_t ret = zeCommandListAppendLaunchKernel(
                    list.handle, kernel, &launch_args, nullptr, 0, nullptr);
                if (ret != ZE_RESULT_SUCCESS) {
                    throw std::runtime_error(
                        std::string("cannot zeCommandListAppendLaunchKernel, error: ") +
                        std::to_string(ret));
                }
                val++;

                thread_lock.unlock();

                // sync and make sure all threads have arrived up to this point.
                while (val < num_thread) {
                }

                // let thread 0 to be the one submitting commands to the queue and sync
                if (thread_idx == 0) {
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
                          "Exception in thread: " << thread_idx << "\nError: " << ex.what()
                                                  << ", at pahse: " << out.str());
                throw;
            }
        });

        thread_out_put.push_back(std::move(out_ptr));
    }

    size_t index = 0;
    for (auto& t : thread_group) {
        t.join();
        output << "Kernels argument binding log for Thread: " << index << std::endl;
        output << thread_out_put[index]->str() << std::endl;
        index++;
    }

    //printout
    output << "Send memory:" << std::endl;
    memory_storage.dump_by_index(output, 0 /*secv_mem*/);
    output << "\nRecv memory:" << std::endl;
    memory_storage.dump_by_index(output, 1 /*recv_mem*/);
    //flags_storage.dump(output);
}
} // namespace singledevice_case
