#pragma once

#include "common/api_wrapper/ze_api_wrapper.hpp"

#include <CL/sycl.hpp>
#include <CL/sycl/backend_types.hpp>
#include <CL/sycl/backend/level_zero.hpp>

#ifdef SYCL_LANGUAGE_VERSION
#define DPCPP_VERSION __clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__
#else // SYCL_LANGUAGE_VERSION
#define DPCPP_VERSION 0
#endif // SYCL_LANGUAGE_VERSION

class ccl_stream;

namespace ccl {
namespace utils {

bool is_sycl_event_completed(sycl::event event);
bool should_use_sycl_output_event(const ccl_stream* stream);

std::string usm_type_to_str(sycl::usm::alloc type);
std::string sycl_device_to_str(const sycl::device& dev);

constexpr sycl::backend get_level_zero_backend() {
#if DPCPP_VERSION >= 140000
    return sycl::backend::ext_oneapi_level_zero;
#elif DPCPP_VERSION < 140000
    return sycl::backend::level_zero;
#endif // DPCPP_VERSION
}

sycl::event submit_barrier(sycl::queue queue);
sycl::event submit_barrier(sycl::queue queue, sycl::event event);

#ifdef CCL_ENABLE_SYCL_INTEROP_EVENT
sycl::event make_event(const sycl::context& context, const ze_event_handle_t& sync_event);
#endif // CCL_ENABLE_SYCL_INTEROP_EVENT

ze_event_handle_t get_native_event(sycl::event event);

} // namespace utils
} // namespace ccl
