#pragma once
#include "ccl.h"
#include "common/utils/utils.hpp"
#include "common/stream/stream_provider_dispatcher.hpp"

namespace ccl
{
    class environment;  //friend-zone
}

ccl_status_t CCL_API ccl_stream_create(ccl_stream_type_t type,
                                          void* native_stream,
                                          ccl_stream_t* stream);

class alignas(CACHELINE_SIZE) ccl_stream : public stream_provider_dispatcher
{
public:
    friend class stream_provider_dispatcher;
    friend class ccl::environment;
    friend ccl_status_t CCL_API ccl_stream_create(ccl_stream_type_t type,
                               void* native_stream,
                               ccl_stream_t* stream);
    using stream_native_t = stream_provider_dispatcher::stream_native_t;
    using stream_native_handle_t = stream_provider_dispatcher::stream_native_handle_t;

    ccl_stream() = delete;
    ccl_stream(const ccl_stream& other) = delete;
    ccl_stream& operator=(const ccl_stream& other) = delete;

    ~ccl_stream() = default;

    using stream_provider_dispatcher::get_native_stream;
    using stream_provider_dispatcher::get_native_stream_handle;

    ccl_stream_type_t get_type() const
    {
        return type;
    }

    static std::unique_ptr<ccl_stream> create(stream_native_t& native_stream);
private:
    template<class NativeStream>
    ccl_stream(ccl_stream_type_t stream_type, NativeStream& native_stream) :
        stream_provider_dispatcher(native_stream),
        type(stream_type)
    {
    }

    ccl_stream_type_t type;
};
