#pragma once
#include "oneapi/ccl/ccl_types.hpp"

#define CL_BACKEND_TYPE ccl::cl_backend_type::l0

#include "oneapi/ccl/native_device_api/l0/declarations.hpp"

namespace ccl
{

template <>
struct backend_info<CL_BACKEND_TYPE> {
    CCL_API static constexpr ccl::cl_backend_type type() {
        return CL_BACKEND_TYPE; }
    CCL_API static constexpr const char* name() {
        return "CL_INTEL_L0_BACKEND"; }
};

template <>
struct generic_device_type<CL_BACKEND_TYPE> {
    using handle_t = device_index_type;
    using impl_t = native::ccl_device;
    using ccl_native_t = std::shared_ptr<impl_t>;

    generic_device_type(device_index_type id);
    generic_device_type(ccl_native_t dev);
    device_index_type get_id() const noexcept;
    ccl_native_t& get() noexcept;
    const ccl_native_t& get() const noexcept;

    handle_t device;
};

template <>
struct generic_context_type<CL_BACKEND_TYPE> {
    using handle_t = ze_context_handle_t;
    using impl_t = native::ccl_context;
    using ccl_native_t = std::shared_ptr<impl_t>;

    generic_context_type();
    generic_context_type(ccl_native_t ctx);
    ccl_native_t& get() noexcept;
    const ccl_native_t& get() const noexcept;

    ccl_native_t context;
};

template <>
struct generic_platform_type<CL_BACKEND_TYPE> {
    using handle_t = native::ccl_device_platform;
    using impl_t = handle_t;
    using ccl_native_t = impl_t;

    ccl_native_t& get() noexcept;
    const ccl_native_t& get() const noexcept;
};

template <>
struct generic_stream_type<CL_BACKEND_TYPE> {
    using handle_t = ze_command_queue_handle_t;
    using impl_t = native::ccl_device::device_queue;
    using ccl_native_t = std::shared_ptr<impl_t>;

    generic_stream_type(handle_t q);
    ccl_native_t& get() noexcept;
    const ccl_native_t& get() const noexcept;

    ccl_native_t queue;
};

template <>
struct generic_event_type<CL_BACKEND_TYPE> {
    using handle_t = ze_event_handle_t;
    using impl_t = handle_t;
    using ccl_native_t = std::shared_ptr<native::ccl_device::device_event>;

    generic_event_type(handle_t e);
    ccl_native_t& get() noexcept;
    const ccl_native_t& get() const noexcept;

    ccl_native_t event;
};

/**
 * Export CL native API supported types
 */
API_CLASS_TYPE_INFO(native::ccl_device::device_queue);
//API_CLASS_TYPE_INFO(ze_command_queue_handle_t);
API_CLASS_TYPE_INFO(ze_event_handle_t);
}
