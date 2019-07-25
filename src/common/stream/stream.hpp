#pragma once

#include "ccl_types.h"
#include "common/utils/utils.hpp"

class alignas(CACHELINE_SIZE) ccl_stream
{
public:
    ccl_stream() = delete;
    ccl_stream(const ccl_stream& other) = delete;
    ccl_stream& operator=(const ccl_stream& other) = delete;

    ~ccl_stream() = default;

    ccl_stream(ccl_stream_type_t stream_type, void *native_stream);

    ccl_stream_type_t get_type() const
    {
        return stream_type;
    }

    ccl_stream_t get_stream() const
    {
        return native_stream;
    }

private:
    ccl_stream_type_t stream_type;
    void *native_stream;
};
