#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "common/stream/stream.hpp"
#include "common/stream/stream_provider_dispatcher_impl.hpp"
#include "common/utils/enums.hpp"
#include "oneapi/ccl/native_device_api/export_api.hpp"

namespace ccl {
std::string to_string(device_family family) {
    switch (family) {
        case device_family::family1: return "family1";
        case device_family::family2: return "family2";
        default: return "unknown";
    }
}
} // namespace ccl

std::string to_string(const stream_type& type) {
    using stream_str_enum = utils::enum_to_str<utils::enum_to_underlying(stream_type::last_value)>;
    return stream_str_enum({ "host", "cpu", "gpu" }).choose(type, "unknown");
}

ccl_stream::ccl_stream(stream_type type,
                       stream_native_t& stream,
                       const ccl::library_version& version)
        : version(version),
          type(type),
          device_family(ccl::device_family::unknown) {
    native_stream = stream;

#ifdef CCL_ENABLE_SYCL
    cl::sycl::property_list props{};
    // TODO: can we somehow simplify this?
    if (stream.is_in_order()) {
        if (ccl::global_data::env().enable_kernel_profile) {
            props = { sycl::property::queue::in_order{},
                      sycl::property::queue::enable_profiling{} };
        }
        else {
            props = { sycl::property::queue::in_order{} };
        }
    }
    else {
        if (ccl::global_data::env().enable_kernel_profile) {
            props = { sycl::property::queue::enable_profiling{} };
        }
        else {
            props = {};
        }
    }

    native_streams.resize(ccl::global_data::env().worker_count);
    for (size_t idx = 0; idx < native_streams.size(); idx++) {
        native_streams[idx] = stream_native_t(stream.get_context(), stream.get_device(), props);
    }

    backend = stream.get_device().get_backend();
#endif // CCL_ENABLE_SYCL

#ifdef CCL_ENABLE_ZE
    if (backend == sycl::backend::level_zero) {
        auto sycl_device = native_stream.get_device();
        auto ze_device = sycl_device.template get_native<sycl::backend::level_zero>();
        device_family = ccl::ze::get_device_family(ze_device);
    }
#endif // CCL_ENABLE_ZE
}

// export attributes
typename ccl_stream::version_traits_t::type ccl_stream::set_attribute_value(
    typename version_traits_t::type val,
    const version_traits_t& t) {
    (void)t;
    throw ccl::exception("set value for 'ccl::stream_attr_id::library_version' is not allowed");
    return version;
}

const typename ccl_stream::version_traits_t::return_type& ccl_stream::get_attribute_value(
    const version_traits_t& id) const {
    return version;
}

typename ccl_stream::native_handle_traits_t::return_type& ccl_stream::get_attribute_value(
    const native_handle_traits_t& id) {
    return native_stream;
}

std::string ccl_stream::to_string() const {
    std::stringstream ss;
#ifdef CCL_ENABLE_SYCL
    ss << "{ "
       << "type: " << ::to_string(type) << ", in_order: " << native_stream.is_in_order()
       << ", device: " << native_stream.get_device().get_info<cl::sycl::info::device::name>()
       << ", device_family: " << ccl::to_string(device_family) << " }";
#else // CCL_ENABLE_SYCL
    ss << reinterpret_cast<void*>(native_stream.get());
#endif // CCL_ENABLE_SYCL
    return ss.str();
}
