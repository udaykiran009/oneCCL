#include "common/comm/l0/context/scaling_ctx/ipc_ctx_session.hpp"
#include "common/comm/l0/context/scaling_ctx/ipc_ctx_utils.hpp"

#include "oneapi/ccl/native_device_api/l0/primitives_impl.hpp"

namespace native {

session::session(origin_ipc_memory_container&& ipc_src_memory_handles,
                 size_t source_ipc_device_rank) {}

session::~session() {}

void session::start(net::ipc_client* client, const std::string& addr) {}

std::string session::to_string() const {
    return { "stub" };
}

bool session::process(const ccl_ipc_gpu_comm* indexed_ipc_dst_devices,
                      const net::ipc_rx_connection* incoming_connection) {
    return true;
}

void session_table::start_session(std::shared_ptr<session> sess,
                                  net::ipc_client* client,
                                  const std::string& peer_addr) {}
} // namespace native
