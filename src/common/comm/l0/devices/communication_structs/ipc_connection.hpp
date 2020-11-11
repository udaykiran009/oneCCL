#pragma once
#include <vector>
#include "oneapi/ccl/native_device_api/l0/device.hpp"
#include "common/comm/l0/devices/communication_structs/connection.hpp"

namespace net {

// Rx receive IPC data from IPC_SOURCE_DEVICE
class ipc_rx_connection : public connection {
public:
    explicit ipc_rx_connection(int socket);

    // For properly call resize data with *_resized before for expected sizes
    std::vector<std::shared_ptr<native::ccl_device::device_ipc_memory_handle>> receive_ipc_memory(
        std::vector<uint8_t>& out_data_resized,
        size_t& out_received_rank) const;

    // For properly call resize data with *_resized before for expected sizes
    std::vector<std::shared_ptr<native::ccl_device::device_ipc_memory_handle>>
    receive_ipc_memory_ext(std::vector<uint8_t>& out_data_resized,
                           size_t out_data_offset_bytes) const;

    // For properly call resize data with *_resized before for expected sizes
    std::vector<std::shared_ptr<native::ccl_device::device_ipc_memory_handle>>
    receive_raw_ipc_memory(std::vector<uint8_t>& out_data_resized,
                           std::vector<connection::fd_t>& out_pids_resized,
                           size_t& out_rank) const;

    // For properly call resize data with *_resized before for expected sizes
    std::vector<std::shared_ptr<native::ccl_device::device_ipc_memory_handle>>
    receive_raw_ipc_memory_ext(std::vector<uint8_t>& out_data_resized,
                               std::vector<connection::fd_t>& out_pids_resized,
                               size_t out_data_offset_bytes) const;
};

// Tx receive IPC data from IPC_SOURCE_DEVICE
class ipc_tx_connection : public connection {
public:
    explicit ipc_tx_connection(const std::string& addr);

    std::vector<uint8_t> send_ipc_memory(
        const std::vector<native::ccl_device::device_ipc_memory_handle>& handles,
        size_t send_rank) const;
    std::vector<uint8_t> send_ipc_memory_ext(
        const std::vector<native::ccl_device::device_ipc_memory_handle>& handles,
        const uint8_t* payload = nullptr,
        size_t payload_size = 0) const;

private:
    sockaddr_un peer_addr{};
};
} // namespace net
