#pragma once

#ifdef CCL_ENABLE_ZE
#include <ze_api.h>
#endif // CCL_ENABLE_ZE

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#endif // CCL_ENABLE_SYCL

#include "oneapi/ccl/type_traits.hpp"

/**
 * Supported stream types
 */
enum class stream_type : int {
    host = 0,
    cpu,
    gpu,

    last_value
};

class ccl_stream;
class stream_provider_dispatcher {
public:
    using stream_native_t = typename ccl::unified_stream_type::ccl_native_t;

    using stream_native_device_t = typename ccl::unified_device_type::ccl_native_t;
    using stream_native_context_t = typename ccl::unified_context_type::ccl_native_t;

    stream_native_t get_native_stream() const;

#ifdef CCL_ENABLE_SYCL
    stream_native_t* get_native_stream(size_t idx);
#endif // CCL_ENABLE_SYCL

    const stream_native_device_t& get_native_device() const;
    stream_native_device_t& get_native_device();

    static std::unique_ptr<ccl_stream> create(stream_native_t& native_stream,
                                              const ccl::library_version& version);
    template <class T>
    using optional = std::pair<bool, T>;

protected:
    optional<stream_native_device_t> native_device;
    optional<stream_native_context_t> native_context;

    stream_native_t native_stream;

#ifdef CCL_ENABLE_SYCL
    /* FIXME: tmp w/a for MT support in queue */
    std::vector<stream_native_t> native_streams;
#endif // CCL_ENABLE_SYCL
};
