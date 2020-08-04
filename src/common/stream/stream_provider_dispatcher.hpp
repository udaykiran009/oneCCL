#pragma once
#ifdef MULTI_GPU_SUPPORT
#include <ze_api.h>
#endif

#ifdef CCL_ENABLE_SYCL
    #include <CL/sycl.hpp>
#endif

#include "ccl_type_traits.hpp"

class ccl_stream;
/*
#ifdef MULTI_GPU_SUPPORT
namespace native
{
    class ccl_device;
}
#endif
*/

class stream_provider_dispatcher
{
public:
#ifdef MULTI_GPU_SUPPORT
    #ifdef CCL_ENABLE_SYCL
        using stream_native_t = cl::sycl::queue;
        using stream_native_device_t = typename ccl::unified_device_type::native_t;
        using stream_native_context_t = typename ccl::unified_device_context_type::native_t;

        using stream_native_handle_t = ze_command_queue_handle_t;//cl::sycl::queue::cl_command_queue;
    #else
        using stream_native_t = ze_command_queue_handle_t;
        using stream_native_device_t = typename ccl::unified_device_type::native_reference_t;
        using stream_native_context_t = typename ccl::unified_device_context_type::native_reference_t;

        using stream_native_handle_t = ze_command_queue_handle_t;
    #endif
#else
    #ifdef CCL_ENABLE_SYCL
        using stream_native_t = cl::sycl::queue;
        using stream_native_handle_t = void *;

        using stream_native_device_t = cl::sycl::device;
    #else
        using stream_native_t = void *;
        using stream_native_handle_t = stream_native_t;

        using stream_native_device_t = void *;
    #endif
#endif
    static stream_native_handle_t get_native_stream_handle_impl(stream_native_t& handle);

    stream_native_t get_native_stream() const;
    stream_native_handle_t get_native_stream_handle() const;

    const stream_native_device_t& get_native_device() const;
    stream_native_device_t& get_native_device();

    std::string to_string() const;

    template <class NativeStream,
              typename std::enable_if<std::is_class<typename std::remove_cv<NativeStream>::type>::value,
                                      int>::type = 0>
    static std::unique_ptr<ccl_stream> create(NativeStream& native_stream, const ccl_version_t& version);

    template <class NativeStream,
              typename std::enable_if<not std::is_class<typename std::remove_cv<NativeStream>::type>::value,
                                      int>::type = 0>
    static std::unique_ptr<ccl_stream> create(NativeStream& native_stream, const ccl_version_t& version);

    static std::unique_ptr<ccl_stream> create(stream_native_device_t device, const ccl_version_t& version);
    static std::unique_ptr<ccl_stream> create(stream_native_device_t device,
                                              stream_native_context_t context,
                                              const ccl_version_t& version);

    static std::unique_ptr<ccl_stream> create( const ccl_version_t& version);

protected:
    template <class NativeStream,
              typename std::enable_if<std::is_class<typename std::remove_cv<NativeStream>::type>::value,
                                      int>::type = 0>
    stream_provider_dispatcher(NativeStream& stream);
    template <class NativeStream,
              typename std::enable_if<not std::is_class<typename std::remove_cv<NativeStream>::type>::value,
                                      int>::type = 0>
    stream_provider_dispatcher(NativeStream stream);

    stream_native_device_t native_device;
    stream_native_context_t native_context;
    bool creation_is_postponed {false};

protected:
    stream_native_t native_stream;
    bool native_stream_set {false}; //TODO use std::variant in c++17

#ifdef CCL_ENABLE_SYCL
    stream_native_handle_t native_stream_handle;
#endif
};
