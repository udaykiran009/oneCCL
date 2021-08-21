#pragma once

#ifdef CCL_ENABLE_SYCL

#include <CL/sycl.hpp>

#include "common/stream/stream.hpp"
#include "common/global/global.hpp"

namespace ccl {
namespace utils {

static inline bool is_sycl_event_completed(sycl::event e) {
    return e.template get_info<sycl::info::event::command_execution_status>() ==
           sycl::info::event_command_status::complete;
}

static inline bool should_use_sycl_output_event(ccl_stream* stream) {
    return (stream && stream->is_sycl_device_stream() && stream->is_gpu() &&
            ccl::global_data::env().enable_kernel_output_event);
}

static inline std::string usm_type_to_str(sycl::usm::alloc type) {
    switch (type) {
        case sycl::usm::alloc::host: return "host";
        case sycl::usm::alloc::device: return "device";
        case sycl::usm::alloc::shared: return "shared";
        case sycl::usm::alloc::unknown: return "unknown";
        default: CCL_THROW("unexpected USM type: ", static_cast<int>(type));
    }
}

static inline std::string sycl_device_to_str(const sycl::device& dev) {
    if (dev.is_host()) {
        return "host";
    }
    else if (dev.is_cpu()) {
        return "cpu";
    }
    else if (dev.is_gpu()) {
        return "gpu";
    }
    else if (dev.is_accelerator()) {
        return "accel";
    }
    else {
        CCL_THROW("unexpected device type");
    }
}

} // namespace utils
} // namespace ccl

#endif // CCL_ENABLE_SYCL
