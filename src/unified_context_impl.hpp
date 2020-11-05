#pragma once

#include "oneapi/ccl/type_traits.hpp"
#include "common/log/log.hpp"

namespace ccl {
#ifdef CCL_ENABLE_SYCL
#ifdef MULTI_GPU_SUPPORT
#else

#endif
#else
#ifdef MULTI_GPU_SUPPORT

#else //MULTI_GPU_SUPPORT

#endif
#endif
} // namespace ccl
