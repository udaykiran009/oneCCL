#pragma once

#include "oneapi/ccl/ccl_environment.hpp"

#include "oneapi/ccl/ccl_api_functions.hpp"

#ifdef MULTI_GPU_SUPPORT
#include "oneapi/ccl/ccl_gpu_modules.h"
#include "oneapi/ccl/native_device_api/l0/context.hpp"
#endif /* MULTI_GPU_SUPPORT */

namespace ccl {}
namespace oneapi {
namespace ccl = ::ccl;
}
