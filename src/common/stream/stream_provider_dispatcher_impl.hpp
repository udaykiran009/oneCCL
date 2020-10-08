#pragma once

#include "common/stream/stream_provider_dispatcher.hpp"

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#endif /* CCL_ENABLE_SYCL */

// Creation from class-type: cl::sycl::queue or native::ccl_device::devie_queue
std::unique_ptr<ccl_stream> stream_provider_dispatcher::create(
    stream_native_t& native_stream,
    const ccl::library_version& version) {

    ccl_stream_type_t type = ccl_stream_host;
#ifdef CCL_ENABLE_SYCL
    if (native_stream.get_device().is_host())
    {
        type = ccl_stream_host;
    }
    else if(native_stream.get_device().is_cpu())
    {
        type = ccl_stream_cpu;
    }
    else if(native_stream.get_device().is_gpu())
    {
        type = ccl_stream_gpu;
    }
    else
    {
        throw ccl::invalid_argument("CORE", "create_stream", std::string("Unsupported SYCL queue's device type:\n") +
                                    native_stream.get_device().template get_info<cl::sycl::info::device::name>() +
                                    std::string("Supported types: host, cpu, gpu"));
    }

    std::unique_ptr<ccl_stream> ret(new ccl_stream(type, native_stream, version));
    ret->native_device.second = native_stream.get_device();
    ret->native_device.first = true;
    ret->native_context.second = native_stream.get_context();
    ret->native_context.first = true;
    LOG_INFO("SYCL queue type: ", static_cast<int>(type), " device: ",
             native_stream.get_device().template get_info<cl::sycl::info::device::name>());

#else
    #ifdef MULTI_GPU_SUPPORT
        LOG_INFO("L0 queue type: gpu - supported only");
        type = ccl_stream_gpu;
        std::unique_ptr<ccl_stream> ret(new ccl_stream(type, native_stream, version));
        ret->native_device.second = native_stream->get_owner().lock();
        ret->native_device.first = true;
        ret->native_context.second = native_stream->get_ctx().lock();
        ret->native_context.first = true;
    #else
        std::unique_ptr<ccl_stream> ret(new ccl_stream(type, native_stream, version));
    #endif
#endif /* CCL_ENABLE_SYCL */

    return ret;
}

// Creation from handles: cl_queue or ze_device_queue_handle_t
std::unique_ptr<ccl_stream> stream_provider_dispatcher::create(
    stream_native_handle_t native_stream,
    const ccl::library_version& version) {
    return std::unique_ptr<ccl_stream>(new ccl_stream(ccl_stream_gpu, native_stream, version));
}

// Postponed creation from device
std::unique_ptr<ccl_stream> stream_provider_dispatcher::create(
    stream_native_device_t device,
    const ccl::library_version& version) {

    auto ret = std::unique_ptr<ccl_stream>(new ccl_stream(ccl_stream_gpu, version));
    ret->native_device.second = device;
    ret->native_device.first = true;
    return ret;
}

// Postponed creation from device & context
std::unique_ptr<ccl_stream> stream_provider_dispatcher::create(
    stream_native_device_t device,
    stream_native_context_t context,
    const ccl::library_version& version) {

    auto ret = stream_provider_dispatcher::create(device, version);
    ret->native_context.second = context;
    ret->native_context.first = true;
    return ret;
}

stream_provider_dispatcher::stream_native_t stream_provider_dispatcher::get_native_stream() const {
    if (creation_is_postponed) {
        throw ccl::exception("native stream is not set");
    }

    return native_stream;
}

const stream_provider_dispatcher::stream_native_device_t&
stream_provider_dispatcher::get_native_device() const {
    if (!native_device.first) {
        throw ccl::exception(std::string(__FUNCTION__) + " - stream has no native device");
    }
    return native_device.second;
}

stream_provider_dispatcher::stream_native_device_t&
stream_provider_dispatcher::get_native_device() {
    return const_cast<stream_provider_dispatcher::stream_native_device_t&>(
        static_cast<const stream_provider_dispatcher*>(this)->get_native_device());
}

std::string stream_provider_dispatcher::to_string() const {
    if (creation_is_postponed) {
        throw ccl::exception("stream is not properly created yet");
    }
    std::stringstream ss;
#ifdef CCL_ENABLE_SYCL
    ss << "sycl: "
       << native_stream.get_info<cl::sycl::info::queue::device>()
              .get_info<cl::sycl::info::device::name>();
#else
    ss << reinterpret_cast<void*>(native_stream.get()); //TODO
#endif
    return ss.str();
}

/*
stream_provider_dispatcher::stream_native_handle_t
stream_provider_dispatcher::get_native_stream_handle_impl(stream_native_t &handle)
{
#ifdef CCL_ENABLE_SYCL
    if (!handle.get_device().is_host())
    {
        return *reinterpret_cast<stream_native_handle_t*>(handle.get());
    }
    else
    {
        return *reinterpret_cast<stream_native_handle_t*>(&handle);
    }
#else
        return handle;
#endif
}
*/
