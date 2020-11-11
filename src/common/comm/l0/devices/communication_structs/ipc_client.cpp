#include <stdexcept>
#include <fcntl.h>
#include <algorithm>

#include "common/log/log.hpp"
#include "common/comm/l0/devices/communication_structs/ipc_client.hpp"
#include "common/comm/l0/devices/communication_structs/ipc_connection.hpp"

namespace net {

ipc_client::~ipc_client() {
    stop_all();
}

std::shared_ptr<ipc_tx_connection> ipc_client::create_connection(const std::string& addr) {

    LOG_DEBUG("Create or find existing connection to: ", addr);
    auto it = connections.find(addr);
    if (it != connections.end()) {
        LOG_DEBUG("Get existing conenction");
        return it->second;
    }

    std::shared_ptr<ipc_tx_connection> tx_conn;
    try {
        tx_conn.reset(new ipc_tx_connection(addr));
    }
    catch (const std::exception& ex) {
        LOG_ERROR("Cannot create TX connection to other IPC server on: ", addr,
                  ", error: ", ex.what());
        throw;
    }

    connections.emplace(addr, tx_conn);

    LOG_DEBUG("Connections created, total tx connections: ", connections.size());
    return tx_conn;
}

bool ipc_client::stop_all() {

    LOG_DEBUG("Stop connections: ", connections.size());
    for(auto &conn_pair : connections) {
        LOG_DEBUG("schedule stop connection to: ", conn_pair.first);
        conn_pair.second.reset();
    }
    return true;
}
}
