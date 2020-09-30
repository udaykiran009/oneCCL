#pragma once

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/log/log.hpp"

namespace ccl {
#ifdef CCL_ENABLE_SYCL
#else
#ifdef MULTI_GPU_SUPPORT

#endif //MULTI_GPU_SUPPORT
#endif
} // namespace ccl
