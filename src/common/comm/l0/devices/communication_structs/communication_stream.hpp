#pragma once
#include <atomic>
#include "oneapi/ccl/native_device_api/export_api.hpp"

namespace native {
struct communication_stream
{
    communication_stream(ccl_device& device, std::shared_ptr<ccl_context> ctx);

    std::shared_ptr<ccl_device::device_queue> device_queue;
    std::atomic<size_t> referenced_communication_device_count;
};


}
