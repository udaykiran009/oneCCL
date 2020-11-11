#pragma once
#include <vector>
#include "oneapi/ccl/native_device_api/l0/device.hpp"

namespace native {
namespace utils {
size_t serialize_ipc_handles(const std::vector<ccl_device::device_ipc_memory_handle>& in_ipc_memory,
                             std::vector<uint8_t>& out_raw_data,
                             size_t out_raw_data_initial_offset_bytes);
/*
    size_t deserialize_ipc_handles(const std::vector<ccl_device::device_ipc_memory_handle>& in_ipc_memory,
                                 std::vector<uint8_t>& out_raw_data,
                                 size_t out_raw_data_initial_offset_bytes);*/
} // namespace utils
} // namespace native
