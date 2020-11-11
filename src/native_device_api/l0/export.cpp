#include "oneapi/ccl/config.h"
#if defined(MULTI_GPU_SUPPORT) and !defined(CCL_ENABLE_SYCL)

#include "oneapi/ccl/native_device_api/l0/export.hpp"
#include "common/log/log.hpp"
#include "native_device_api/compiler_ccl_wrappers_dispatcher.hpp"

namespace ccl {

/**
 * Context
 */
CCL_API generic_context_type<cl_backend_type::l0>::generic_context_type() {}
CCL_API generic_context_type<cl_backend_type::l0>::generic_context_type(ccl_native_t ctx) : context(ctx) {}

CCL_API generic_context_type<cl_backend_type::l0>::ccl_native_t&
generic_context_type<cl_backend_type::l0>::get() noexcept {
    return const_cast<generic_context_type<cl_backend_type::l0>::ccl_native_t&>(
        static_cast<const generic_context_type<cl_backend_type::l0>*>(this)->get());
}

CCL_API const generic_context_type<cl_backend_type::l0>::ccl_native_t&
generic_context_type<cl_backend_type::l0>::get() const noexcept {
    //TODO
    return context; //native::get_platform();
}

/**
 * Device
 */
CCL_API generic_device_type<cl_backend_type::l0>::generic_device_type(device_index_type id) : device(id) {}

CCL_API generic_device_type<cl_backend_type::l0>::generic_device_type(ccl_native_t dev)
        : device(dev->get_device_path()) {}

CCL_API device_index_type generic_device_type<cl_backend_type::l0>::get_id() const noexcept {
    return device;
}

CCL_API typename generic_device_type<cl_backend_type::l0>::ccl_native_t
generic_device_type<cl_backend_type::l0>::get() noexcept {
    return native::get_runtime_device(device);
}

CCL_API const typename generic_device_type<cl_backend_type::l0>::ccl_native_t
generic_device_type<cl_backend_type::l0>::get() const noexcept {
    return native::get_runtime_device(device);
}

/**
 * Event
 */
CCL_API generic_event_type<cl_backend_type::l0>::generic_event_type(handle_t e)
        : event(/*TODO use ccl_context to create event*/) {}

CCL_API generic_event_type<cl_backend_type::l0>::ccl_native_t&
generic_event_type<cl_backend_type::l0>::get() noexcept {
    return const_cast<generic_event_type<cl_backend_type::l0>::ccl_native_t&>(
        static_cast<const generic_event_type<cl_backend_type::l0>*>(this)->get());
}

CCL_API const generic_event_type<cl_backend_type::l0>::ccl_native_t&
generic_event_type<cl_backend_type::l0>::get() const noexcept {
    return event;
}

/**
 * Stream
 */
CCL_API generic_stream_type<cl_backend_type::l0>::generic_stream_type(handle_t q)
        : queue(/*TODO use ccl_device to create event*/) {}

CCL_API generic_stream_type<cl_backend_type::l0>::ccl_native_t&
generic_stream_type<cl_backend_type::l0>::get() noexcept {
    return const_cast<generic_stream_type<cl_backend_type::l0>::ccl_native_t&>(
        static_cast<const generic_stream_type<cl_backend_type::l0>*>(this)->get());
}

CCL_API const generic_stream_type<cl_backend_type::l0>::ccl_native_t&
generic_stream_type<cl_backend_type::l0>::get() const noexcept {
    return queue;
}

/**
 * Platform
 */
CCL_API generic_platform_type<cl_backend_type::l0>::ccl_native_t&
generic_platform_type<cl_backend_type::l0>::get() noexcept {
    return const_cast<generic_platform_type<cl_backend_type::l0>::ccl_native_t&>(
        static_cast<const generic_platform_type<cl_backend_type::l0>*>(this)->get());
}

CCL_API const generic_platform_type<cl_backend_type::l0>::ccl_native_t&
generic_platform_type<cl_backend_type::l0>::get() const noexcept {
    return native::get_platform();
}
} // namespace ccl
#endif //MULTI_GPU_SUPPORT
