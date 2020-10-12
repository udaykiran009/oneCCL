#ifndef TRAITS_H_
#define TRAITS_H_

#include <tuple>
#include <type_traits>

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#endif

#include "oneapi/ccl/ccl_types.hpp"

namespace ccl {
/**
 * Base type-trait helpers for "unknown" types
 */
template <ccl::datatype ccl_type_id>
struct type_info {
    static constexpr bool is_supported = false;
    static constexpr bool is_class = false;
};

template <class type>
struct native_type_info {
    static constexpr bool is_supported = false;
    static constexpr bool is_class = false;
};

#define CCL_TYPE_TRAITS(ccl_type_id, dtype, dtype_size, name_in_kernel) \
    template <> \
    struct type_info<ccl_type_id> \
            : public ccl_type_info_export<dtype, dtype_size, ccl_type_id, false, true> { \
        static constexpr const char* name() { \
            return #dtype; \
        } \
        static constexpr const char* name_for_kernel() { \
            return #name_in_kernel; \
        } \
    }; \
    template <> \
    struct native_type_info<dtype> : public type_info<ccl_type_id> {};

#define CCL_CLASS_TYPE_TRAITS(ccl_type_id, dtype, sizeof_dtype, name_in_kernel) \
    template <> \
    struct native_type_info<dtype> \
            : public ccl_type_info_export<dtype, sizeof_dtype, ccl_type_id, true, true> { \
        static constexpr const char* name() { \
            return #dtype; \
        } \
        static constexpr const char* name_for_kernel() { \
            return #name_in_kernel; \
        } \
    };

#define COMMA ,

/*struct bf16_impl
{
    uint16_t data;
} __attribute__((packed));*/

using bf16 = uint16_t;

/**
 * Enumeration of supported CCL API data types
 */
CCL_TYPE_TRAITS(ccl::datatype::int8, char, sizeof(char), int8_t)
CCL_TYPE_TRAITS(ccl::datatype::int32, int, sizeof(int), int32_t)
CCL_TYPE_TRAITS(ccl::datatype::bfloat16, bf16, sizeof(bf16), bf16)
CCL_TYPE_TRAITS(ccl::datatype::float32, float, sizeof(float), float32_t)
CCL_TYPE_TRAITS(ccl::datatype::float64, double, sizeof(double), float64_t)
CCL_TYPE_TRAITS(ccl::datatype::int64, int64_t, sizeof(int64_t), int64_t)
CCL_TYPE_TRAITS(ccl::datatype::uint64, uint64_t, sizeof(uint64_t), uint64_t)

#ifdef CCL_ENABLE_SYCL
CCL_CLASS_TYPE_TRAITS(ccl::datatype::int8, cl::sycl::buffer<char COMMA 1>, sizeof(char), int8_t)
CCL_CLASS_TYPE_TRAITS(ccl::datatype::int32, cl::sycl::buffer<int COMMA 1>, sizeof(int), int32_t)
CCL_CLASS_TYPE_TRAITS(ccl::datatype::bfloat16, cl::sycl::buffer<bf16 COMMA 1>, sizeof(bf16), bf16)
CCL_CLASS_TYPE_TRAITS(ccl::datatype::int64, cl::sycl::buffer<int64_t COMMA 1>, sizeof(int64_t), float32_t)
CCL_CLASS_TYPE_TRAITS(ccl::datatype::uint64, cl::sycl::buffer<uint64_t COMMA 1>, sizeof(uint64_t), float64_t)
CCL_CLASS_TYPE_TRAITS(ccl::datatype::float32, cl::sycl::buffer<float COMMA 1>, sizeof(float), int64_t)
CCL_CLASS_TYPE_TRAITS(ccl::datatype::float64, cl::sycl::buffer<double COMMA 1>, sizeof(double), uint64_t)
#endif //CCL_ENABLE_SYCL

/**
 * Checks for supporting @c type in ccl API
 */
template <class type>
constexpr bool is_supported() {
    using clear_type = typename std::remove_pointer<type>::type;
    //    static_assert(native_type_info<clear_type>::is_supported, "type is not supported by ccl API");
    return native_type_info<clear_type>::is_supported;
}

/**
 * Checks is @c type a class
 */
template <class type>
constexpr bool is_class() {
    using clear_type = typename std::remove_pointer<type>::type;
    return native_type_info<clear_type>::is_class;
}

/**
 * SFINAE checks for supporting native type @c type in ccl API
 */
template <class type>
constexpr bool is_native_type_supported() {
    return (not is_class<type>() and is_supported<type>());
}

/**
  * SFINAE checks for supporting class @c type in ccl API
  */
template <class type>
constexpr bool is_class_supported() {
    return (is_class<type>() and is_supported<type>());
}

} // namespace ccl
#include "oneapi/ccl/ccl_device_type_traits.hpp"
#endif //TRAITS_H_
