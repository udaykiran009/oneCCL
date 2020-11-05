#include "oneapi/ccl/config.h"

#if defined(CCL_ENABLE_SYCL) and !defined(MULTI_GPU_SUPPORT)

#include "oneapi/ccl/native_device_api/sycl/export.hpp"
#include "oneapi/ccl/type_traits.hpp"
#include "common/log/log.hpp"
#include "native_device_api/compiler_ccl_wrappers_dispatcher.hpp"

namespace ccl {

/**
 * Context
 */
generic_context_type<cl_backend_type::dpcpp_sycl>::generic_context_type() {}
generic_context_type<cl_backend_type::dpcpp_sycl>::generic_context_type(ccl_native_t ctx)
        : context(ctx) {}

generic_context_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_context_type<cl_backend_type::dpcpp_sycl>::get() noexcept {
    return const_cast<generic_context_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&>(
        static_cast<const generic_context_type<cl_backend_type::dpcpp_sycl>*>(this)->get());
}

const generic_context_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_context_type<cl_backend_type::dpcpp_sycl>::get() const noexcept {
    return context;
}

/**
 * Device
 */
CCL_API generic_device_type<cl_backend_type::dpcpp_sycl>::generic_device_type(
    device_index_type id,
    cl::sycl::info::device_type type)
        : device() {
    LOG_DEBUG("Try to find SYCL device by index: ",
              id,
              ", type: ",
              static_cast<typename std::underlying_type<cl::sycl::info::device_type>::type>(type));

    auto platforms = cl::sycl::platform::get_platforms();
    LOG_DEBUG("Found CL plalforms: ", platforms.size());
    auto platform_it =
        std::find_if(platforms.begin(), platforms.end(), [](const cl::sycl::platform& pl) {
            return pl.get_info<cl::sycl::info::platform::name>().find("Level-Zero") !=
                   std::string::npos;
            //or platform.get_backend() == cl::sycl::backend::level_zero
        });
    if (platform_it == platforms.end()) {
        std::stringstream ss;
        ss << "cannot find Level-Zero platform. Supported platforms are:\n";
        for (const auto& pl : platforms) {
            ss << "Platform:\nprofile: " << pl.get_info<cl::sycl::info::platform::profile>()
               << "\nversion: " << pl.get_info<cl::sycl::info::platform::version>()
               << "\nname: " << pl.get_info<cl::sycl::info::platform::name>()
               << "\nvendor: " << pl.get_info<cl::sycl::info::platform::vendor>();
        }

        throw std::runtime_error(std::string("Cannot find device by id: ") + ccl::to_string(id) +
                                 ", reason:\n" + ss.str());
    }

    LOG_DEBUG("Platform:\nprofile: ",
              platform_it->get_info<cl::sycl::info::platform::profile>(),
              "\nversion: ",
              platform_it->get_info<cl::sycl::info::platform::version>(),
              "\nname: ",
              platform_it->get_info<cl::sycl::info::platform::name>(),
              "\nvendor: ",
              platform_it->get_info<cl::sycl::info::platform::vendor>());
    auto devices = platform_it->get_devices(type);

    LOG_DEBUG("Found devices: ", devices.size());
    auto it =
        std::find_if(devices.begin(), devices.end(), [id](const cl::sycl::device& dev) -> bool {
            //TODO -S- not implemented
            (void)id;
            return true;
        });
    if (it == devices.end()) {
        std::stringstream ss;
        ss << "cannot find requested device. Supported devices are:\n";
        for (const auto& dev : devices) {
            ss << "Device:\nname: " << dev.get_info<cl::sycl::info::device::name>()
               << "\nvendor: " << dev.get_info<cl::sycl::info::device::vendor>()
               << "\nversion: " << dev.get_info<cl::sycl::info::device::version>()
               << "\nprofile: " << dev.get_info<cl::sycl::info::device::profile>();
        }

        throw std::runtime_error(std::string("Cannot find device by id: ") + ccl::to_string(id) +
                                 ", reason:\n" + ss.str());
    }
    device = *it;
}

generic_device_type<cl_backend_type::dpcpp_sycl>::generic_device_type(
    const cl::sycl::device& in_device)
        : device(in_device) {}

device_index_type generic_device_type<cl_backend_type::dpcpp_sycl>::get_id() const {
    //TODO -S-
    return device_index_type{};
}

typename generic_device_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_device_type<cl_backend_type::dpcpp_sycl>::get() noexcept {
    return device;
}

const typename generic_device_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_device_type<cl_backend_type::dpcpp_sycl>::get() const noexcept {
    return device;
}

/**
 * Event
 */
generic_event_type<cl_backend_type::dpcpp_sycl>::generic_event_type(ccl_native_t ev) : event(ev) {}

generic_event_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_event_type<cl_backend_type::dpcpp_sycl>::get() noexcept {
    return const_cast<generic_event_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&>(
        static_cast<const generic_event_type<cl_backend_type::dpcpp_sycl>*>(this)->get());
}

const generic_event_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_event_type<cl_backend_type::dpcpp_sycl>::get() const noexcept {
    return event;
}

/**
 * Stream
 */
generic_stream_type<cl_backend_type::dpcpp_sycl>::generic_stream_type(ccl_native_t q) : queue(q) {}

generic_stream_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_stream_type<cl_backend_type::dpcpp_sycl>::get() noexcept {
    return const_cast<generic_stream_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&>(
        static_cast<const generic_stream_type<cl_backend_type::dpcpp_sycl>*>(this)->get());
}

const generic_stream_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_stream_type<cl_backend_type::dpcpp_sycl>::get() const noexcept {
    return queue;
}

/**
 * Platform
 */
generic_platform_type<cl_backend_type::dpcpp_sycl>::generic_platform_type(ccl_native_t pl)
        : platform(pl) {}

generic_platform_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_platform_type<cl_backend_type::dpcpp_sycl>::get() noexcept {
    return const_cast<generic_platform_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&>(
        static_cast<const generic_platform_type<cl_backend_type::dpcpp_sycl>*>(this)->get());
}

const generic_platform_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_platform_type<cl_backend_type::dpcpp_sycl>::get() const noexcept {
    return platform;
}
} // namespace ccl
#endif //CCL_ENABLE_SYCL and !defined(MULTI_GPU_SUPPORT)
