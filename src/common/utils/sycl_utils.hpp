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

} // namespace utils
} // namespace ccl
#endif