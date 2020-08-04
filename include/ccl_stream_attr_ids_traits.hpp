#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {
namespace details
{

/**
 * Traits for stream attributes specializations
 */
template <>
struct ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::version> {
    using type = ccl_version_t;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::native_handle> {
    using type = typename unified_stream_type::native_reference_t;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::device> {
    using type = typename unified_device_type::native_reference_t;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::context> {
    using type = typename unified_device_context_type::native_reference_t;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::ordinal> {
    using type = uint32_t;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::index> {
    using type = uint32_t;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::flags> {
    using type = size_t;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::mode> {
    using type = size_t;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<stream_attr_id, stream_attr_id::priority> {
    using type = size_t;
    using return_type = type;
};

}
}
