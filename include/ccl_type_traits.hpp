#ifndef TRAITS_H_
#define TRAITS_H_

#include <tuple>
#include <type_traits>

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#endif

#include "ccl_types.hpp"

namespace ccl {
/**
 * Base type-trait helpers for "unknown" types
 */
template <ccl_datatype_t ccl_type_id>
struct type_info {
    static constexpr bool is_supported = false;
    static constexpr bool is_class = false;
};

template <class type>
struct native_type_info {
    static constexpr bool is_supported = false;
    static constexpr bool is_class = false;
};

#define CCL_TYPE_TRAITS(ccl_type_id, dtype, dtype_size) \
    template <> \
    struct type_info<ccl_type_id> \
            : public ccl_type_info_export<dtype, dtype_size, ccl_type_id, false, true> { \
        static constexpr const char* name() { \
            return #dtype; \
        } \
    }; \
    template <> \
    struct native_type_info<dtype> : public type_info<ccl_type_id> {};

#define CCL_CLASS_TYPE_TRAITS(ccl_type_id, dtype, sizeof_dtype) \
    template <> \
    struct native_type_info<dtype> \
            : public ccl_type_info_export<dtype, sizeof_dtype, ccl_type_id, true, true> { \
        static constexpr const char* name() { \
            return #dtype; \
        } \
    };

#define COMMA ,

/*struct bfp16_impl
{
    uint16_t data;
} __attribute__((packed));*/

using bfp16 = uint16_t;

/**
 * Enumeration of supported CCL API data types
 */
CCL_TYPE_TRAITS(ccl_dtype_char, char, sizeof(char))
CCL_TYPE_TRAITS(ccl_dtype_int, int, sizeof(int))
CCL_TYPE_TRAITS(ccl_dtype_bfp16, bfp16, sizeof(bfp16))
CCL_TYPE_TRAITS(ccl_dtype_float, float, sizeof(float))
CCL_TYPE_TRAITS(ccl_dtype_double, double, sizeof(double))
CCL_TYPE_TRAITS(ccl_dtype_int64, int64_t, sizeof(int64_t))
CCL_TYPE_TRAITS(ccl_dtype_uint64, uint64_t, sizeof(uint64_t))

#ifdef CCL_ENABLE_SYCL
CCL_CLASS_TYPE_TRAITS(ccl_dtype_char, cl::sycl::buffer<char COMMA 1>, sizeof(char))
CCL_CLASS_TYPE_TRAITS(ccl_dtype_int, cl::sycl::buffer<int COMMA 1>, sizeof(int))
CCL_CLASS_TYPE_TRAITS(ccl_dtype_bfp16, cl::sycl::buffer<bfp16 COMMA 1>, sizeof(bfp16))
CCL_CLASS_TYPE_TRAITS(ccl_dtype_int64, cl::sycl::buffer<int64_t COMMA 1>, sizeof(int64_t))
CCL_CLASS_TYPE_TRAITS(ccl_dtype_uint64, cl::sycl::buffer<uint64_t COMMA 1>, sizeof(uint64_t))
CCL_CLASS_TYPE_TRAITS(ccl_dtype_float, cl::sycl::buffer<float COMMA 1>, sizeof(float))
CCL_CLASS_TYPE_TRAITS(ccl_dtype_double, cl::sycl::buffer<double COMMA 1>, sizeof(double))
#endif //CCL_ENABLE_SYCL

template <>
struct comm_split_attributes_traits<ccl_host_color> {
    using type = int;
};

template <>
struct comm_split_attributes_traits<ccl_host_version> {
    using type = ccl_version_t;
};

/**
 * Checks for supporting @c type in ccl API
 */
template <class type>
constexpr bool is_supported() {
    using clear_type = typename std::remove_pointer<type>::type;
    static_assert(native_type_info<clear_type>::is_supported, "type is not supported by ccl API");
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

template <comm_split_attr_id attr_id, class value>
constexpr bool is_attribute_value_supported() {
    return std::is_same<
        typename comm_split_attributes_traits<attr_id>::type,
        typename std::remove_cv<typename std::remove_reference<value>::type>::type>::value;
}

} // namespace ccl
#include "ccl_device_type_traits.hpp"
#endif //TRAITS_H_
