#pragma once
#include <memory>
#include <sstream>

#define HOST_CTX
#include "kernels/event_declaration.h"

#define private public
#define protected public
/**
 * Add custom types support into native::memory example
 */
/* 1) Describe new type traits */
namespace ccl {
template <>
struct native_type_info<shared_event_float> {
    static constexpr bool is_supported = true;
    static constexpr bool is_class = true;
};
} // namespace ccl
/* 2) Include explicit definition for native::memory */
#include "oneapi/ccl/native_device_api/l0/primitives_impl.hpp"

#include "observer_event_impl.hpp"
#undef protected
#undef private

namespace context_event_suite {

// test case data
static const size_t buffer_size = 1024024;

using native_type = float;

TEST_F(shared_context_fixture, observer_event_test) {
    ccl::environment::instance();

    using namespace native;
    
    //TODO
    std::shared_ptr<ccl_context> ctx;
    // check global driver
    auto drv_it = local_platform->drivers.find(0);
    UT_ASSERT(drv_it != local_platform->drivers.end(), "Driver by idx 0 must exist!");
    ccl_device_driver& driver = *drv_it->second;

    // check devices per process
    UT_ASSERT(driver.devices.size() == local_affinity.size(),
              "Count: %" << driver.devices.size() << ", bits: " << local_affinity.size()
                         << "Device count is not equal to affinity mask!");

    // device memory stencil data
    std::vector<native_type> init_data(buffer_size);
    std::iota(init_data.begin(), init_data.end(), 1);
    std::vector<native_type> out_data(buffer_size, 0);

    // allocate device memory
    auto dev_it = driver.devices.begin();
    ccl_device& device = *dev_it->second;

    auto device_init_data = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
    auto device_out_data = device.alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);

    ze_host_mem_alloc_desc_t host_descr;
    //host_descr.
    auto shared_flag = device.alloc_shared_memory<int>(1, sizeof(int), host_descr, ctx);
    auto shared_chunk =
        device.alloc_shared_memory<native_type>(buffer_size, sizeof(native_type), host_descr, ctx);

    // initialize memory
    device_init_data.enqueue_write_sync(init_data);
    device_out_data.enqueue_write_sync(out_data);
    *(shared_flag->get()) = 0;

    //prepare kernels in multithreading environment
    ze_kernel_desc_t desc = {
        .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
        .pNext = nullptr,
        .flags = 0,
        .pKernelName = "numa_poll",
    };

    ccl_device::device_module& module = *(device_modules.find(&device)->second);

    //thread_group.emplace
    ze_kernel_handle_t kernel = nullptr;
    ze_result_t result = zeKernelCreate(module.handle, &desc, &kernel);
    if (result != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("Cannot create kernel: ") + desc.pKernelName +
                                 ", error: " + native::to_string(result));
    }

    ccl_device::device_queue queue = device.create_cmd_queue(ctx);
    ccl_device::device_cmd_list cmd_list = device.create_cmd_list(ctx);

    //Set args and launch kernel
    size_t job_id = 0;
    ze_group_count_t launch_args = { 1, 1, 1 };
    try {
        ze_result_t result;
        output << "job_id: " << job_id << ", args binding: \n";

        int arg_offset = 0;
        output << "index: " << arg_offset << ": " << device_init_data.get() << std::endl;
        native_type* init_mem_handle = device_init_data.get();
        result =
            zeKernelSetArgumentValue(kernel, arg_offset, sizeof(init_mem_handle), &init_mem_handle);
        if (result != ZE_RESULT_SUCCESS) {
            throw std::runtime_error(
                std::string("Cannot zeKernelSetArgumentValue memory at offset: ") +
                std::to_string(arg_offset) + " index\nError: " + native::to_string(result));
        }
        arg_offset++;

        native_type* output_mem_handle = device_out_data.get();
        result = zeKernelSetArgumentValue(
            kernel, arg_offset, sizeof(output_mem_handle), &output_mem_handle);
        if (result != ZE_RESULT_SUCCESS) {
            throw std::runtime_error(
                std::string("Cannot zeKernelSetArgumentValue memory at offset: ") +
                std::to_string(arg_offset) + " index\nError: " + native::to_string(result));
        }
        arg_offset++;

        int* shared_flag_mem_handle = shared_flag->get();
        result = zeKernelSetArgumentValue(
            kernel, arg_offset, sizeof(shared_flag_mem_handle), &shared_flag_mem_handle);
        if (result != ZE_RESULT_SUCCESS) {
            throw std::runtime_error(
                std::string("Cannot zeKernelSetArgumentValue memory at offset: ") +
                std::to_string(arg_offset) + " index\nError: " + native::to_string(result));
        }
        arg_offset++;

        native_type* shared_mem_handle = shared_chunk->get();
        result = zeKernelSetArgumentValue(
            kernel, arg_offset, sizeof(shared_mem_handle), &shared_mem_handle);
        if (result != ZE_RESULT_SUCCESS) {
            throw std::runtime_error(
                std::string("Cannot zeKernelSetArgumentValue memory at offset: ") +
                std::to_string(arg_offset) + " index\nError: " + native::to_string(result));
        }
        arg_offset++;

        /* Hints for memory allocation
        {
            //set indirect access for another peer device buffer

            result = zeKernelSetAttribute(kernel, ze_kernel_attribute_t::ZE_KERNEL_ATTR_INDIRECT_DEVICE_ACCESS, sizeof(flag), &flag));
            if (result != ZE_RESULT_SUCCESS)
            {
                    throw std::runtime_error(std::string("Cannot zeKernelSetAttribute flags at flag_offset: ") +
                    std::to_string(flag_offset[i]) + " index\nError: " +
                                native::to_string(result));
            }
        }
        */

        result = zeCommandListAppendLaunchKernel(
            cmd_list.handle, kernel, &launch_args, nullptr, 0, nullptr);
        if (result != ZE_RESULT_SUCCESS) {
            throw std::runtime_error(
                std::string("cannot zeCommandListAppendLaunchKernel, error: ") +
                std::to_string(result));
        }

        result = zeCommandListClose(cmd_list.handle);
        if (result != ZE_RESULT_SUCCESS) {
            throw std::runtime_error(std::string("cannot zeCommandListClose, error: ") +
                                     std::to_string(result));
        }
    }
    catch (const std::exception& ex) {
        UT_ASSERT(false, "Exception in thread: " << job_id << "\nError: " << ex.what());
        throw;
    }

    // lauch consumer
    std::shared_ptr<observer_event<native_type>> entry =
        std::make_shared<observer_event<native_type>>(
            nullptr, shared_chunk, shared_flag, buffer_size);
    std::thread consumer_thread([&]() {
        auto cnt = entry->bytes_consumed;
        entry->start();
        while (!entry->is_completed()) {
            if (cnt != entry->bytes_consumed) {
                output << "consumed bytes: " << entry->bytes_consumed << std::endl;
                cnt = entry->bytes_consumed;
            }
            entry->update();
        }
    });

    result = zeCommandQueueExecuteCommandLists(queue.handle, 1, &cmd_list.handle, nullptr);
    if (result != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("cannot zeCommandQueueExecuteCommandLists, error: ") +
                                 std::to_string(result));
    }

    result = zeCommandQueueSynchronize(queue.handle, std::numeric_limits<uint32_t>::max());
    if (result != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("cannot zeCommandQueueSynchronize, error: ") +
                                 std::to_string(result));
    }

    // wait
    consumer_thread.join();
    int* consumed = shared_flag->get();
    output << "consumed bytes: " << *consumed << std::endl;
    output << "consumed data: " << std::endl;
    /* for (size_t i = 0; i < buffer_size; i++)
    {
        output << *(shared_chunk->get() + i) << ", ";
    }*/
}

#if 0
TEST_F(shared_context_fixture, observer_event_structure_test)
{
    ccl::environment::instance();

    using namespace native;

    // check global driver
    auto drv_it = local_platform->drivers.find(0);
    UT_ASSERT(drv_it != local_platform->drivers.end(),
              "Driver by idx 0 must exist!");
    ccl_device_driver& driver = *drv_it->second;

    // check devices per process
    UT_ASSERT(driver.devices.size() == local_affinity.size(), "Count: %" <<  driver.devices.size()
              << ", bits: " << local_affinity.size()
              << "Device count is not equal to affinity mask!");

    // device memory stencil data
    std::vector<native_type> init_data(buffer_size);
    std::iota(init_data.begin(), init_data.end(), 1);
    std::vector<native_type> out_data(buffer_size, 0);

    // allocate device memory
    auto dev_it = driver.devices.begin();
    ccl_device& device = *dev_it->second;

    auto device_init_data = device.alloc_memory<native_type>(buffer_size, sizeof(native_type));
    auto device_out_data = device.alloc_memory<native_type>(buffer_size, sizeof(native_type));

    ze_host_mem_alloc_desc_t host_descr;

    auto shared_event = device.alloc_shared_memory<shared_event_float>(1, sizeof(shared_event_float),
                                                                       host_descr);
    auto shared_flag = device.alloc_shared_memory<int>(1, sizeof(int),
                                                          host_descr);
    auto shared_chunk = device.alloc_shared_memory<native_type>(buffer_size, sizeof(native_type),
                                                                host_descr);


    //std::unique_ptr<shared_event_float> host_event_filler(new shared_event_float);
    //host_event_filler->produced_bytes = shared_flag->get();
    //host_event_filler->mem_chunk = shared_chunk->get();
    //memcpy(shared_event->handle, host_event_filler.get(), sizeof(shared_event_float));
    shared_event->handle->produced_bytes = shared_flag->handle;
    shared_event->handle->mem_chunk = shared_chunk->handle;

    // initialize memory
    device_init_data.enqueue_write_sync(init_data);
    device_out_data.enqueue_write_sync(out_data);
    *(shared_event->handle->produced_bytes) = 10;

    //prepare kernels in multithreading environment
    ze_kernel_desc_t desc = { ZE_KERNEL_DESC_VERSION_CURRENT,
                              ZE_KERNEL_FLAG_NONE   };
    desc.pKernelName = "numa_poll";
    ccl_device::device_module& module  = *(device_modules.find(&device)->second);

    //thread_group.emplace
    ze_kernel_handle_t kernel = nullptr;
    ze_result_t result = zeKernelCreate(module.handle, &desc, &kernel);
    if (result != ZE_RESULT_SUCCESS)
    {
        throw std::runtime_error(std::string("Cannot create kernel: ") +
                                 desc.pKernelName +
                                 ", error: " + native::to_string(result));
    }

    ccl_device::device_queue queue = device.create_cmd_queue();
    ccl_device::device_cmd_list cmd_list = device.create_cmd_list();

    //Set args and launch kernel
    size_t job_id = 0;
    ze_group_count_t launch_args = { 1, 1, 1 };
    try
    {
        ze_result_t result;
        output << "job_id: " << job_id << ", args binding: \n";

        int arg_offset = 0;
        output << "index: " << arg_offset << ": " << device_init_data.get() << std::endl;
        native_type* init_mem_handle = device_init_data.get();
        result = zeKernelSetArgumentValue(kernel, arg_offset, sizeof(init_mem_handle), &init_mem_handle);
        if (result != ZE_RESULT_SUCCESS)
        {
            throw std::runtime_error(std::string("Cannot zeKernelSetArgumentValue memory at offset: ") +
                                                std::to_string(arg_offset) + " index\nError: " +
                                                native::to_string(result));
        }
        arg_offset++;

        native_type* output_mem_handle = device_out_data.get();
        output << "index: " << arg_offset << ": " << output_mem_handle << std::endl;
        result = zeKernelSetArgumentValue(kernel, arg_offset, sizeof(output_mem_handle), &output_mem_handle);
        if (result != ZE_RESULT_SUCCESS)
        {
            throw std::runtime_error(std::string("Cannot zeKernelSetArgumentValue memory at offset: ") +
                                                std::to_string(arg_offset) + " index\nError: " +
                                                native::to_string(result));
        }
        arg_offset++;

        //shared_event_float* shared_flag_handle = shared_event->handle;

        *(shared_event->handle->produced_bytes) = 999;
        *shared_flag->handle = 888;
        output << "index: " << arg_offset << ": " << shared_event->handle << std::endl;
        result = zeKernelSetArgumentValue(kernel, arg_offset,
                                          sizeof(shared_event->handle),
                                          &(shared_event->handle));
        if (result != ZE_RESULT_SUCCESS)
        {
            throw std::runtime_error(std::string("Cannot zeKernelSetArgumentValue memory at offset: ") +
                                                std::to_string(arg_offset) + " index\nError: " +
                                                native::to_string(result));
        }
        arg_offset++;

        /* Hints for memory allocation
        {
            //set indirect access for another peer device buffer

            result = zeKernelSetAttribute(kernel, ze_kernel_attribute_t::ZE_KERNEL_ATTR_INDIRECT_DEVICE_ACCESS, sizeof(flag), &flag));
            if (result != ZE_RESULT_SUCCESS)
            {
                    throw std::runtime_error(std::string("Cannot zeKernelSetAttribute flags at flag_offset: ") +
                    std::to_string(flag_offset[i]) + " index\nError: " +
                                native::to_string(result));
            }
        }
        */

        result = zeCommandListAppendLaunchKernel(cmd_list.handle, kernel,
                                                 &launch_args, nullptr, 0, nullptr);
        if (result != ZE_RESULT_SUCCESS )
        {
            throw std::runtime_error(std::string("cannot zeCommandListAppendLaunchKernel, error: ") +
                                     std::to_string(result));
        }

        result = zeCommandListClose(cmd_list.handle);
        if (result != ZE_RESULT_SUCCESS )
        {
            throw std::runtime_error(std::string("cannot zeCommandListClose, error: ") +
                                     std::to_string(result));
        }
    }
    catch(const std::exception& ex)
    {
        UT_ASSERT(false, "Exception in thread: " << job_id << "\nError: " <<ex.what());
        throw;
    }

    // lauch consumer
    std::shared_ptr<observer_event<native_type>> entry =
            std::make_shared<observer_event<native_type>>(nullptr,
                                                          shared_event,
                                                          buffer_size);
    std::thread consumer_thread([&]()
    {
        output << "starting pollier thread" << std::endl;
        auto cnt = entry->bytes_consumed;
        entry->start();
        while (!entry->is_completed())
        {
            if (cnt != entry->bytes_consumed)
            {
                output << "consumed bytes: " << entry->bytes_consumed << std::endl;
                cnt = entry->bytes_consumed;
            }
            entry->update();
        }
    });

    result = zeCommandQueueExecuteCommandLists(queue.handle, 1, &cmd_list.handle, nullptr);
    if (result != ZE_RESULT_SUCCESS )
    {
        throw std::runtime_error(std::string("cannot zeCommandQueueExecuteCommandLists, error: ") +
                                 std::to_string(result));
    }

    result = zeCommandQueueSynchronize(queue.handle, std::numeric_limits<uint32_t>::max());
    if (result != ZE_RESULT_SUCCESS )
    {
        throw std::runtime_error(std::string("cannot zeCommandQueueSynchronize, error: ") +
                                 std::to_string(result));
    }

    // wait
    consumer_thread.join();
    int *consumed = shared_flag->get();
    output << "consumed bytes: " << *consumed << std::endl;
    output << "consumed data: " << std::endl;
   /* for (size_t i = 0; i < buffer_size; i++)
    {
        output << *(shared_chunk->get() + i) << ", ";
    }*/
}

#endif
} // namespace context_event_suite
