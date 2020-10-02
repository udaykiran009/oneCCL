#pragma once
#include <memory>
#include <sstream>

#include "allreduce_fixture.hpp"

namespace ring_single_device_case {

namespace ring_allreduce_case {

using bf16 = ushort;
using float32_t = float;
using float64_t = double;

template <class T>
struct my_add {
    T operator()(const T& lhs, const T& rhs) const {
        return lhs + rhs;
    }

    static constexpr T init = 0;
};

template <class T>
struct my_mult {
    T operator()(const T& lhs, const T& rhs) const {
        return lhs * rhs;
    }

    static constexpr T init = 1;
};

template <class T>
struct my_min {
    T operator()(const T& lhs, const T& rhs) const {
        return std::min(lhs, rhs);
    }

    static constexpr T init = std::numeric_limits<T>::max();
};

template <class T>
struct my_max {
    T operator()(const T& lhs, const T& rhs) const {
        return std::max(lhs, rhs);
    }

    static constexpr T init = std::numeric_limits<T>::min();
};

#define DEFINE_PAIR(T, Op) \
    std::pair<T, Op>

#define DEFINE_TYPE(T)          \
    DEFINE_PAIR(T, my_add<T>),  \
    DEFINE_PAIR(T, my_mult<T>), \
    DEFINE_PAIR(T, my_max<T>),  \
    DEFINE_PAIR(T, my_min<T>)

// For test we use enumerate over all the types and the ops and get pairs of <T, Op>.
using TestTypes = ::testing::Types<
// Reduce the number of tested typed in order to speedup test
/*  DEFINE_TYPE(int8_t),
    DEFINE_TYPE(uint8_t),
    DEFINE_TYPE(int16_t),
    DEFINE_TYPE(uint16_t),
    DEFINE_TYPE(int32_t),
    DEFINE_TYPE(uint32_t),
    DEFINE_TYPE(int64_t),
    DEFINE_TYPE(uint64_t),
//  DEFINE_TYPE(float16_t),
    DEFINE_TYPE(float32_t),
    DEFINE_TYPE(float64_t),
    DEFINE_TYPE(bf16) */
    DEFINE_PAIR(int8_t, my_add<int8_t>),
    DEFINE_PAIR(uint8_t, my_mult<uint8_t>),
    DEFINE_PAIR(int16_t, my_min<int16_t>),
    DEFINE_PAIR(uint16_t, my_max<uint16_t>),
    DEFINE_PAIR(int32_t, my_add<int32_t>),
    DEFINE_PAIR(uint32_t, my_mult<uint32_t>),
    DEFINE_PAIR(int64_t, my_min<int64_t>),
    DEFINE_PAIR(uint64_t, my_max<uint64_t>),
//  DEFINE_PAIR(float16_t, my_add<float16_t>),
    DEFINE_PAIR(float32_t, my_mult<float32_t>),
    DEFINE_PAIR(float64_t, my_min<float64_t>),
    DEFINE_PAIR(bf16, my_max<bf16>)
>;

template <class T, class Op>
struct op_traits {
    static constexpr size_t buffer_size = 512;
};

template <class T, class Op>
struct param_traits;

#define DEFINE_KERNEL_TYPE(T, Op) \
    template <> \
    struct param_traits<T, my_##Op<T>> { \
        static constexpr const char* kernel_name = "allreduce_execution_" # T "_" #Op; \
        using op_type = my_##Op<T>; \
    };

#define DEFINE_KERNEL_TYPES_FOR_OP(OpName)              \
    DEFINE_KERNEL_TYPE(int8_t, OpName)                  \
    DEFINE_KERNEL_TYPE(uint8_t, OpName)                 \
                                                        \
    DEFINE_KERNEL_TYPE(int16_t, OpName)                 \
    DEFINE_KERNEL_TYPE(uint16_t, OpName)                \
                                                        \
    DEFINE_KERNEL_TYPE(int32_t, OpName)                 \
    DEFINE_KERNEL_TYPE(uint32_t, OpName)                \
                                                        \
    DEFINE_KERNEL_TYPE(int64_t, OpName)                 \
    DEFINE_KERNEL_TYPE(uint64_t, OpName)                \
                                                        \
    /*DEFINE_KERNEL_TYPE(float16_t, OpName)*/           \
    DEFINE_KERNEL_TYPE(float32_t, OpName)               \
    DEFINE_KERNEL_TYPE(float64_t, OpName)               \
    /* TODO: Enable once bf16 is not aliased to ushort  \
        or remove if it's kept as this */               \
    /*DEFINE_KERNEL_TYPE(bf16, OpName)*/                \

DEFINE_KERNEL_TYPES_FOR_OP(add);
DEFINE_KERNEL_TYPES_FOR_OP(mult);
DEFINE_KERNEL_TYPES_FOR_OP(min);
DEFINE_KERNEL_TYPES_FOR_OP(max);

#undef DEFINE_KERNEL_TYPE
#undef DEFINE_TYPE
#undef DEFINE_PAIR
#undef DEFINE_KERNEL_TYPES_FOR_OP

}

TYPED_TEST_CASE(ring_allreduce_single_device_fixture, ring_allreduce_case::TestTypes);

TYPED_TEST(ring_allreduce_single_device_fixture, ring_allreduce_single_device_mt) {
    using namespace native;

    // Type of our test
    using native_type = typename TypeParam::first_type;
    using op_type = typename TypeParam::second_type;

    // test case data
    const size_t buffer_size = 512 * 4;
    const size_t num_thread = 4;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;

    handles_storage<native_type> memory_storage(mem_group_count * num_thread);
    handles_storage<int> flags_storage(flag_group_count * num_thread);
    std::map<size_t, std::vector<size_t>> comm_param_storage;

    ccl_device_driver::create_context() ctx;

    // check global driver
    auto drv_it = this->local_platform->drivers.find(0);
    UT_ASSERT(drv_it != this->local_platform->drivers.end(), "Driver by idx 0 must exist!");
    ccl_device_driver& driver = *drv_it->second;

    // check devices per process
    UT_ASSERT(driver.devices.size() == this->local_affinity.size(),
              "Count: %" << driver.devices.size() << ", bits: " << this->local_affinity.size()
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
            auto mem_send = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
            auto mem_recv = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);

            /* FIXME: use 2x size for tmp buffer for parallel recv and reduce+send */
            /* consider to remove tmp buffer further */
            auto temp_recv =
                device.alloc_memory<native_type>(2 * buffer_size / num_thread, sizeof(native_type), ctx);

            mem_send.enqueue_write_sync(send_values);
            mem_recv.enqueue_write_sync(recv_values);
            temp_recv.enqueue_write_sync(recv_values.begin(),
                                         recv_values.begin() + 2 * buffer_size / num_thread);

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
            auto left_wrote_2_me_flag = device.alloc_memory<int>(1, sizeof(int), ctx);
            auto ready_for_receive_flag = device.alloc_memory<int>(1, sizeof(int), ctx);
            auto barrier_flag = device.alloc_memory<int>(1, sizeof(int), ctx);
            left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
            ready_for_receive_flag.enqueue_write_sync({ (int)0 });
            barrier_flag.enqueue_write_sync({ (int)0 });

            /* fill array in specific order
             * Left: l_L, l_R, l_B, r_L, r_R
             * Right: r_L, r_R, r_B, l_L, L_R
             */
            flags_storage.register_shared_data(thread_idx,
                                               num_thread,
                                               std::move(left_wrote_2_me_flag),
                                               std::move(ready_for_receive_flag),
                                               std::move(barrier_flag));
        }
        catch (const std::exception& ex) {
            UT_ASSERT(
                false,
                "Cannot allocate memory for thread: " << thread_idx << "\nError: " << ex.what());
        }
    }

    for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
        memory_storage.rotate_shared_data(thread_idx, num_thread, mem_group_count);
        flags_storage.rotate_shared_data(thread_idx, num_thread, flag_group_count);
    }
    //Check memory handles
    /*
    try
    {
        for(const auto& alloc_pair : thread_allocated_memory_storage)
        {
            const mem_handles_container& mem_handles = alloc_pair.second;
            for(const auto& mem : mem_handles)
            {
                //TODO
                //device.is_own_memory(mem);
                ze_memory_allocation_properties_t mem_prop;
                mem_prop.version = ZE_MEMORY_ALLOCATION_PROPERTIES_VERSION_CURRENT;
                ze_device_handle_t alloc_device_handle{};
                ze_result_t result = zeDriverGetMemAllocProperties(driver.handle, mem->handle, &mem_prop, &alloc_device_handle);
                if (result != ZE_RESULT_SUCCESS)
                {
                    throw std::runtime_error(std::string("Cannot zeDriverGetMemAllocProperties at: ") +
                                             native::to_string(result));
                }
                if(alloc_device_handle and alloc_device_handle != device.handle)
                {
                    throw std::runtime_error(std::string("Device handle for memory allocations mismatch detected"));
                }
            }
        }
    }
    catch(const std::exception& ex)
    {
        UT_ASSERT(false, "Check allocated memory failed: %s",
                  ex.what());
    }
    */
    //prepare kernels in multithreading environment
    ze_kernel_desc_t desc = {
        .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
        .pNext = nullptr,
        .flags = 0,
    };
    desc.pKernelName = ring_allreduce_case::param_traits<native_type, op_type>::kernel_name;
    std::map<size_t, ze_kernel_handle_t> thread_kernels;
    std::map<size_t, ccl_device::device_queue> thread_queue;
    std::map<size_t, ccl_device::device_cmd_list> thread_cmd_list;
    ccl_device::device_module& module = *(this->device_modules.find(&device)->second);
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
            thread_queue.emplace(thread_idx, device.create_cmd_queue(ctx));
            thread_cmd_list.emplace(thread_idx, device.create_cmd_list(ctx));
        }
        catch (const std::exception& ex) {
            throw std::runtime_error(std::string("Error: ") + ex.what());
        }
    }

    //printout
    auto& out = this->output;
    out << "L0 memory handles: " << std::endl;
    memory_storage.dump(out, true);

    //Set args and launch kernel
    std::mutex thread_lock; //workaround
    std::atomic<size_t> val{ 0 }; //workaround
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    for (auto& idx_kernel : thread_kernels) {
        size_t thread_idx = idx_kernel.first;
        ze_kernel_handle_t kernel = idx_kernel.second;
        auto& mem_handles = memory_storage.per_thread_storage.find(thread_idx)->second;
        auto& flag_handles = flags_storage.per_thread_storage.find(thread_idx)->second;
        auto& comm_handles = comm_param_storage.find(thread_idx)->second;
        //WORKAROUND: ONLY ONE LIST & QUEUE!
        //ccl_device::device_queue& queue = thread_queue.find(thread_idx)->second;
        ccl_device::device_queue& queue = thread_queue.find(0)->second;
        //ccl_device::device_cmd_list& list = thread_cmd_list.find(thread_idx)->second;
        ccl_device::device_cmd_list& list = thread_cmd_list.find(0)->second;

        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();
        thread_group.emplace_back([this,
                                   &driver,
                                   &device,
                                   thread_idx,
                                   kernel,
                                   &list,
                                   &queue,
                                   &mem_handles,
                                   &flag_handles,
                                   &comm_handles,
                                   &thread_lock,
                                   &val,
                                   raw_out]() {
            (void)driver;
            (void)device;
            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { 1, 1, 1 };
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
                std::array<int, mem_group_count * 2> mem_offset{ 3, 4, 5, -1, -1, 9 };
                //UT_ASSERT(mem_offset.size() == mem_handles.size(), "mem_offset != mem_handles");
                out << "thread_idx: " << thread_idx << ", mem_handles: \n";
                for (auto& mem : mem_handles) {
                    if (i >= mem_group_count * 2) {
                        break; //only own+right is needed
                    }
                    if (mem_offset[i] == -1) {
                        i++;
                        continue; //skip this argument
                    }

                    out << "index: " << mem_offset[i] << ": " << (void*)mem << std::endl;
                    result = zeKernelSetArgumentValue(kernel, mem_offset[i], sizeof(mem), &mem);
                    if (result != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("Cannot zeKernelSetArgumentValue memory at mem_offset: ") +
                            std::to_string(mem_offset[i]) +
                            " index\nError: " + native::to_string(result));
                    }

                    /* Hints for memory allocation*/
                    if (mem_offset[i] == 9) {
                        //set indirect access for another peer device buffer
                        /* TODO
                        result = zeKernelSetAttribute(kernel, ze_kernel_attribute_t::ZE_KERNEL_ATTR_INDIRECT_DEVICE_ACCESS, sizeof(mem), &mem));
                        if (result != ZE_RESULT_SUCCESS)
                        {
                            throw std::runtime_error(std::string("Cannot zeKernelSetAttribute memory at mem_offset: ") +
                                                std::to_string(mem_offset[i]) + " index\nError: " +
                                                native::to_string(result));
                        }*/
                    }
                    i++;
                }
                out << std::endl;

                // bindleft_wrote_2_me_flag, ready_for_receive_flag, local_barrier_flag
                i = 0;
                std::array<int, flag_group_count * 2> flag_offset{ 6, 7, 8, 10, 11, -1 };
                //UT_ASSERT(flag_offset.size() == flag_handles.size(), "flag_offset != flag_handles");
                out << "thread_idx: " << thread_idx << ", flag_handles: \n";
                for (auto& flag : flag_handles) {
                    if (i >= flag_group_count * 2) {
                        break; //only own+right is needed
                    }

                    if (flag_offset[i] == -1) {
                        i++;
                        continue; //skip this argument
                    }
                    out << "index: " << flag_offset[i] << ": " << flag << std::endl;
                    result = zeKernelSetArgumentValue(kernel, flag_offset[i], sizeof(flag), &flag);
                    if (result != ZE_RESULT_SUCCESS) {
                        throw std::runtime_error(
                            std::string("Cannot zeKernelSetArgumentValue flags at flag_offset: ") +
                            std::to_string(flag_offset[i]) +
                            " index\nError: " + native::to_string(result));
                    }

                    /* Hints for memory allocation*/
                    if (flag_offset[i] == 7 or flag_offset[i] == 8) {
                        //set indirect access for another peer device buffer
                        /* TODO
                        result = zeKernelSetAttribute(kernel, ze_kernel_attribute_t::ZE_KERNEL_ATTR_INDIRECT_DEVICE_ACCESS, sizeof(flag), &flag));
                        if (result != ZE_RESULT_SUCCESS)
                        {
                            throw std::runtime_error(std::string("Cannot zeKernelSetAttribute flags at flag_offset: ") +
                                                std::to_string(flag_offset[i]) + " index\nError: " +
                                                native::to_string(result));
                        }
                        */
                    }
                    i++;
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
        out << "Kernels argument binding log for Thread: " << index << std::endl;
        out << thread_out_put[index]->str() << std::endl;
        index++;
    }

    size_t corr_val;
    try {
        for (auto& idx_kernel : thread_kernels) {
            size_t thread_idx = idx_kernel.first;
            corr_val = 0;

            auto lambda = [&corr_val](
                              size_t thread_idx, size_t num_thread, native_type value) -> bool {
                corr_val++;

                constexpr auto op = op_type{};

                native_type totalVal = op.init;
                for (size_t i = 0; i < num_thread; ++i) {
                    totalVal = op(totalVal, static_cast<native_type>(corr_val));
                }

                if (!(value == totalVal)) {
                    return false;
                }

                return true;
            };

            memory_storage.check_results(
                thread_idx, out, 1 /*recv_mem*/, lambda, thread_idx, num_thread);
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
        ss << ex.what() << ", But expected: " << corr_val * num_thread << std::endl;
        UT_ASSERT(false, ss.str());
    }
}

} // namespace ring_single_device_case