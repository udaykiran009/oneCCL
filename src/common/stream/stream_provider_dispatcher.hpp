#pragma once

#ifdef CCL_ENABLE_ZE
#include "common/ze/ze_api_wrapper.hpp"
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

    stream_native_t get_native_stream() const;

#ifdef CCL_ENABLE_SYCL
    stream_native_t* get_native_stream(size_t idx);
#endif // CCL_ENABLE_SYCL

    static std::unique_ptr<ccl_stream> create(stream_native_t& native_stream,
                                              const ccl::library_version& version);

protected:
    stream_native_t native_stream;

#ifdef CCL_ENABLE_SYCL
    /* FIXME: tmp w/a for MT support in queue */
    std::vector<stream_native_t> native_streams;
#endif // CCL_ENABLE_SYCL
};
