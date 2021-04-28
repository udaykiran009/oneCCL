#pragma once

#include "configuration.hpp"

#define private public
#define protected public
#define MULTI_GPU_SUPPORT
#include "oneapi/ccl/native_device_api/export_api.hpp"
#include "oneapi/ccl/native_device_api/l0/declarations.hpp"

#undef protected
#undef private

#ifdef STANDALONE_UT
#define UT_ASSERT(cond, ...) \
    do { \
        if (!(cond)) { \
            std::cerr << __VA_ARGS__ << std::endl; \
            this->set_error(__PRETTY_FUNCTION__); \
            dump(); \
            abort(); \
        } \
    } while (0);

#define UT_ASSERT_GLOBAL(cond, ...) \
    do { \
        if (!(cond)) { \
            std::cerr << __VA_ARGS__ << std::endl; \
            abort(); \
        } \
    } while (0);
#else
#define UT_ASSERT_OBJ(cond, obj, ...) \
    do { \
        if (!(cond)) { \
            obj->set_error(__PRETTY_FUNCTION__); \
        } \
        { ASSERT_TRUE((cond)) << __VA_ARGS__ << std::endl; } \
    } while (0);

#define UT_ASSERT(cond, ...) UT_ASSERT_OBJ(cond, this, __VA_ARGS__)

#define UT_ASSERT_GLOBAL(cond, ...) \
    do { \
        { ASSERT_TRUE((cond)) << __VA_ARGS__ << std::endl; } \
    } while (0);
#endif
