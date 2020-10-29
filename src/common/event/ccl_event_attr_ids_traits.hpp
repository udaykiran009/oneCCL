#pragma once

namespace ccl {

namespace detail {

/**
 * Traits for stream attributes specializations
 */
template <>
struct ccl_api_type_attr_traits<event_attr_id, event_attr_id::version> {
    using type = ccl::library_version;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<event_attr_id, event_attr_id::native_handle> {
    using type = typename unified_event_type::ccl_native_t;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<event_attr_id, event_attr_id::context> {
    using type = typename unified_context_type::ccl_native_t;
    using handle_t = typename unified_context_type::ccl_native_t;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<event_attr_id, event_attr_id::command_type> {
    using type = uint32_t;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<event_attr_id, event_attr_id::command_execution_status> {
    using type = int64_t;
    using return_type = type;
};

} // namespace detail
} // namespace ccl
