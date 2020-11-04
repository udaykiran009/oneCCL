#include "oneapi/ccl/ccl_config.h"
#if defined(MULTI_GPU_SUPPORT) and !defined(CCL_ENABLE_SYCL)

#include "oneapi/ccl/native_device_api/l0/export.hpp"
#include "common/log/log.hpp"
#include "native_device_api/compiler_ccl_wrappers_dispatcher.hpp"

namespace ccl {

/**
 * Context
 */
generic_context_type<cl_backend_type::l0>::generic_context_type() {}
generic_context_type<cl_backend_type::l0>::generic_context_type(ccl_native_t ctx) : context(ctx) {}

generic_context_type<cl_backend_type::l0>::ccl_native_t&
generic_context_type<cl_backend_type::l0>::get() noexcept {
    return const_cast<generic_context_type<cl_backend_type::l0>::ccl_native_t&>(
        static_cast<const generic_context_type<cl_backend_type::l0>*>(this)->get());
}

const generic_context_type<cl_backend_type::l0>::ccl_native_t&
generic_context_type<cl_backend_type::l0>::get() const noexcept {
    //TODO
    return context; //native::get_platform();
}

/**
 * Device
 */
generic_device_type<cl_backend_type::l0>::generic_device_type(device_index_type id) : device(id) {}

generic_device_type<cl_backend_type::l0>::generic_device_type(ccl_native_t dev)
        : device(dev->get_device_path()) {}

device_index_type generic_device_type<cl_backend_type::l0>::get_id() const noexcept {
    return device;
}

typename generic_device_type<cl_backend_type::l0>::ccl_native_t&
generic_device_type<cl_backend_type::l0>::get() noexcept {
    return native::get_runtime_device(device);
}

const typename generic_device_type<cl_backend_type::l0>::ccl_native_t&
generic_device_type<cl_backend_type::l0>::get() const noexcept {
    return native::get_runtime_device(device);
}

/**
 * Event
 */
generic_event_type<cl_backend_type::l0>::generic_event_type(handle_t e)
        : event(/*TODO use ccl_context to create event*/) {}

generic_event_type<cl_backend_type::l0>::ccl_native_t&
generic_event_type<cl_backend_type::l0>::get() noexcept {
    return const_cast<generic_event_type<cl_backend_type::l0>::ccl_native_t&>(
        static_cast<const generic_event_type<cl_backend_type::l0>*>(this)->get());
}

const generic_event_type<cl_backend_type::l0>::ccl_native_t&
generic_event_type<cl_backend_type::l0>::get() const noexcept {
    return event;
}

/**
 * Stream
 */
generic_stream_type<cl_backend_type::l0>::generic_stream_type(handle_t q)
        : queue(/*TODO use ccl_device to create event*/) {}

generic_stream_type<cl_backend_type::l0>::ccl_native_t&
generic_stream_type<cl_backend_type::l0>::get() noexcept {
    return const_cast<generic_stream_type<cl_backend_type::l0>::ccl_native_t&>(
        static_cast<const generic_stream_type<cl_backend_type::l0>*>(this)->get());
}

const generic_stream_type<cl_backend_type::l0>::ccl_native_t&
generic_stream_type<cl_backend_type::l0>::get() const noexcept {
    return queue;
}

/**
 * Platform
 */
generic_platform_type<cl_backend_type::l0>::ccl_native_t&
generic_platform_type<cl_backend_type::l0>::get() noexcept {
    return const_cast<generic_platform_type<cl_backend_type::l0>::ccl_native_t&>(
        static_cast<const generic_platform_type<cl_backend_type::l0>*>(this)->get());
}

const generic_platform_type<cl_backend_type::l0>::ccl_native_t&
generic_platform_type<cl_backend_type::l0>::get() const noexcept {
    return native::get_platform();
}
} // namespace ccl
#endif //MULTI_GPU_SUPPORT
