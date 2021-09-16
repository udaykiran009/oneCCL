#pragma once
#include "oneapi/ccl/config.h"

#ifdef CCL_ENABLE_SYCL
#ifdef CCL_ENABLE_ZE
#include "sycl_l0/export.hpp"
#else
#include "sycl/export.hpp"
#endif
#else
#ifdef CCL_ENABLE_ZE
#include "l0/export.hpp"
#else
#include "empty/export.hpp"
#endif
#endif

#ifndef CL_BACKEND_TYPE
#error "Unsupported CL_BACKEND_TYPE. Available backends are: dpcpp_sycl, l0 "
#endif
namespace ccl {
using backend_traits = backend_info<CL_BACKEND_TYPE>;
using unified_device_type = generic_device_type<CL_BACKEND_TYPE>;
using unified_context_type = generic_context_type<CL_BACKEND_TYPE>;
using unified_platform_type = generic_platform_type<CL_BACKEND_TYPE>;
using unified_stream_type = generic_stream_type<CL_BACKEND_TYPE>;
using unified_event_type = generic_event_type<CL_BACKEND_TYPE>;
} // namespace ccl

#include "interop_utils.hpp"
