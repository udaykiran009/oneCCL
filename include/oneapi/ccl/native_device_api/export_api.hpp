#pragma once

#ifdef MULTI_GPU_SUPPORT
    #include "l0/export.hpp"
#else
    #ifndef CCL_ENABLE_SYCL
        #include "empty/export.hpp"
    #endif
#endif

#include "interop_utils.hpp"
