#pragma once

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)

#include "common/ze/ze_api_wrapper.hpp"
#include <CL/sycl.hpp>
#include <CL/sycl/backend/level_zero.hpp>

#include "common/stream/stream.hpp"
#include "common/global/global.hpp"

#ifdef SYCL_LANGUAGE_VERSION
#define DPCPP_VERSION __clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__
#else // SYCL_LANGUAGE_VERSION
#define DPCPP_VERSION 0
#endif // SYCL_LANGUAGE_VERSION

namespace ccl {
namespace utils {

static inline bool is_sycl_event_completed(sycl::event e) {
    return e.template get_info<sycl::info::event::command_execution_status>() ==
           sycl::info::event_command_status::complete;
}

static inline bool should_use_sycl_output_event(ccl_stream* stream) {
    return (stream && stream->is_sycl_device_stream() && stream->is_gpu() &&
            ccl::global_data::env().enable_sycl_output_event);
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
#if DPCPP_VERSION >= 140000
    return sycl::backend::ext_oneapi_level_zero;
#elif DPCPP_VERSION < 140000
    return sycl::backend::level_zero;
#endif // DPCPP_VERSION
}

static inline sycl::event submit_barrier(cl::sycl::queue queue) {
#if DPCPP_VERSION >= 140000
    return queue.ext_oneapi_submit_barrier();
#elif DPCPP_VERSION < 140000
    return queue.submit_barrier();
#endif // DPCPP_VERSION
}

static inline sycl::event submit_barrier(cl::sycl::queue queue, sycl::event event) {
#if DPCPP_VERSION >= 140000
    return queue.ext_oneapi_submit_barrier({ event });
#elif DPCPP_VERSION < 140000
    return queue.submit_barrier({ event });
#endif // DPCPP_VERSION
}

#ifdef CCL_ENABLE_SYCL_INTEROP_EVENT
static inline sycl::event make_event(sycl::context& context, ze_event_handle_t& sync_event) {
#if DPCPP_VERSION >= 140000
    return sycl::make_event<sycl::backend::ext_oneapi_level_zero>(
        { sync_event, sycl::ext::oneapi::level_zero::ownership::keep }, context);
#elif DPCPP_VERSION < 140000
    return sycl::level_zero::make<sycl::event>(
        context, sync_event, sycl::level_zero::ownership::keep);
#endif // DPCPP_VERSION
}
#endif // CCL_ENABLE_SYCL_INTEROP_EVENT

} // namespace utils
} // namespace ccl

#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL
