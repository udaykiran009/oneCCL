#pragma once

/* All symbols shall be internal unless marked as CCL_API */
#ifdef __linux__
#   if __GNUC__ >= 4
#       define CCL_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
#   else
#       define CCL_HELPER_DLL_EXPORT
#   endif
#else
#error "unexpected OS"
#endif

#define CCL_API CCL_HELPER_DLL_EXPORT

#define CCL_MAJOR_VERSION                   0
#define CCL_MINOR_VERSION                   8
#define CCL_UPDATE_VERSION                  0
#define CCL_PRODUCT_STATUS             "beta"
#define CCL_PRODUCT_BUILD_DATE         "2020-09-10T 14:55:44Z"
#define CCL_PRODUCT_FULL               "beta-0.8.0 2020-09-10T 14:55:44Z (new_api_bugfix/b91bd71)"


/* Configuration settings for multi GPU extension support*/
#define MULTI_GPU_SUPPORT

/* Auto-generated configuration settings for SYCL support */
#define CCL_ENABLE_SYCL

#ifdef CCL_ENABLE_SYCL
#if defined(__cplusplus)
#if !defined(__clang__) || __clang_major__ < 9 || !defined(CL_SYCL_LANGUAGE_VERSION)
#error This version of CCL configured only for DPC++ compiler
#endif
#endif
#endif

#define CCL_ENABLE_SYCL_V              1
#define CCL_ENABLE_SYCL_TRUE                1
#define CCL_ENABLE_SYCL_FALSE               0
#define CCL_GPU_DEVICES_AFFINITY_ENABLE
#define CCL_GPU_DEVICES_AFFINITY_MASK_SIZE 4
