#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl_type_traits.hpp'"
#endif

#include  "oneapi/ccl/native_device_api/export_api.hpp"

namespace ccl {

#define SUPPORTED_KERNEL_NATIVE_DATA_TYPES char, int, float, ccl::bf16, double, int64_t, uint64_t

template <class native_stream>
constexpr bool is_stream_supported() {
    return api_type_info</*typename std::remove_pointer<typename std::remove_cv<*/
                         native_stream /*>::type>::type*/>::is_supported();
}

template <class native_event>
constexpr bool is_event_supported() {
    return api_type_info</*typename std::remove_pointer<typename std::remove_cv<*/
                         native_event /*>::type>::type*/>::is_supported();
}

template <class native_device>
constexpr bool is_device_supported() {
    return api_type_info<typename std::remove_pointer<typename std::remove_cv<
                         typename std::remove_reference<native_device>::type>::type>::type>::is_supported();
}

template <class native_context>
constexpr bool is_context_supported() {
    return api_type_info<typename std::remove_pointer<typename std::remove_cv<
                         typename std::remove_reference<native_context>::type>::type>::type>::is_supported();
}

/**
 * Export common native API supported types
 */
API_CLASS_TYPE_INFO(empty_t);
API_CLASS_TYPE_INFO(typename unified_device_type::ccl_native_t)
API_CLASS_TYPE_INFO(typename unified_context_type::ccl_native_t);
API_CLASS_TYPE_INFO(typename unified_stream_type::ccl_native_t);
API_CLASS_TYPE_INFO(typename unified_event_type::ccl_native_t);

//TMP - matching device index into native device object
template <class... Args>
unified_device_type create_from_index(Args&&... args) {
    return unified_device_type(std::forward<Args>(args)...);
}
} // namespace ccl
