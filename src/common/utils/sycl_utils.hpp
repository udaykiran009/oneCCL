#pragma once

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)

#include <ze_api.h>
#include <CL/sycl.hpp>
#include <CL/sycl/backend/level_zero.hpp>

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

constexpr sycl::backend get_level_zero_backend() {
#ifdef CCL_ENABLE_LATEST_DPCPP
    return sycl::backend::ext_oneapi_level_zero;
#else // CCL_ENABLE_LATEST_DPCPP
    return sycl::backend::level_zero;
#endif // CCL_ENABLE_LATEST_DPCPP
}

static inline sycl::event submit_barrier(cl::sycl::queue queue,
                                         std::shared_ptr<sycl::event> ev_ptr = nullptr) {
    sycl::event event;
#ifdef CCL_ENABLE_LATEST_DPCPP
    if (ev_ptr) {
        event = queue.ext_oneapi_submit_barrier({ *ev_ptr.get() });
    }
    else {
        event = queue.ext_oneapi_submit_barrier();
    }
#else // CCL_ENABLE_LATEST_DPCPP
    if (ev_ptr) {
        event = queue.submit_barrier({ *ev_ptr.get() });
    }
    else {
        event = queue.submit_barrier();
    }
#endif // CCL_ENABLE_LATEST_DPCPP
    return event;
}

static inline sycl::event make_event(sycl::context& context, ze_event_handle_t& sync_event) {
#ifdef CCL_ENABLE_LATEST_DPCPP
    return sycl::make_event<sycl::backend::ext_oneapi_level_zero>(
        { sync_event, sycl::ext::oneapi::level_zero::ownership::keep }, context);
#else // CCL_ENABLE_LATEST_DPCPP
    return sycl::level_zero::make<sycl::event>(
        context, sync_event, sycl::level_zero::ownership::keep);
#endif // CCL_ENABLE_LATEST_DPCPP
}

} // namespace utils
} // namespace ccl

#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL
