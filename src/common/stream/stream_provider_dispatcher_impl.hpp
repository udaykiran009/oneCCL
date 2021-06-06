#pragma once

#include "common/stream/stream_provider_dispatcher.hpp"

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#endif /* CCL_ENABLE_SYCL */

// creation from sycl::queue
std::unique_ptr<ccl_stream> stream_provider_dispatcher::create(
    stream_native_t& native_stream,
    const ccl::library_version& version) {
    stream_type type = stream_type::host;

#ifdef CCL_ENABLE_SYCL
    if (native_stream.get_device().is_host()) {
        type = stream_type::host;
    }
    else if (native_stream.get_device().is_cpu()) {
        type = stream_type::cpu;
    }
    else if (native_stream.get_device().is_gpu()) {
        type = stream_type::gpu;
    }
    else {
        throw ccl::invalid_argument(
            "core",
            "create_stream",
            std::string("unsupported SYCL queue's device type:\n") +
                native_stream.get_device().template get_info<cl::sycl::info::device::name>() +
                std::string("supported types: host, cpu, gpu"));
    }

    std::unique_ptr<ccl_stream> ret(new ccl_stream(type, native_stream, version));
    ret->native_device.second = native_stream.get_device();
    ret->native_device.first = true;
    ret->native_context.second = native_stream.get_context();
    ret->native_context.first = true;

    LOG_INFO("SYCL queue type: ",
             ::to_string(type),
             ", in_order: ",
             native_stream.is_in_order(),
             ", device: ",
             native_stream.get_device().template get_info<cl::sycl::info::device::name>());

#else /* CCL_ENABLE_SYCL */
    std::unique_ptr<ccl_stream> ret(new ccl_stream(type, native_stream, version));
#endif /* CCL_ENABLE_SYCL */

    return ret;
}

stream_provider_dispatcher::stream_native_t stream_provider_dispatcher::get_native_stream() const {
    return native_stream;
}

#ifdef CCL_ENABLE_SYCL
stream_provider_dispatcher::stream_native_t* stream_provider_dispatcher::get_native_stream(
    size_t idx) {
    if (idx >= native_streams.size()) {
        throw ccl::exception("unexpected stream idx");
    }
    return &(native_streams[idx]);
}
#endif /* CCL_ENABLE_SYCL */
