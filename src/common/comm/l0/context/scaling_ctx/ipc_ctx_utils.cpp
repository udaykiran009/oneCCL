#include "common/log/log.hpp"
#include "common/comm/l0/context/scaling_ctx/ipc_ctx_utils.hpp"
#include "common/comm/l0/devices/devices_declaration.hpp"

namespace native {
namespace utils {
size_t serialize_ipc_handles(const std::vector<ccl_device::device_ipc_memory_handle>& in_ipc_memory,
                             std::vector<uint8_t>& out_raw_data,
                             size_t out_raw_data_initial_offset_bytes) {
    // serialize data for native allgather algo
    out_raw_data.clear();
    constexpr size_t handle_size = ccl_device::device_ipc_memory_handle::get_size_for_serialize();

    size_t send_bytes =
        handle_size * in_ipc_memory.size() +
        out_raw_data_initial_offset_bytes; //ipc_data + out_raw_data_initial_offset_bytes
    out_raw_data.resize(send_bytes);

    // fill send_buf
    size_t serialize_offset = out_raw_data_initial_offset_bytes;
    for (const auto& ipc_handle : in_ipc_memory) {
        serialize_offset += ipc_handle.serialize(out_raw_data, serialize_offset);
    }

    CCL_ASSERT(serialize_offset == send_bytes,
               "Expected data to send and actually serialized are differ");

    return send_bytes;
}
} // namespace utils
} // namespace native
