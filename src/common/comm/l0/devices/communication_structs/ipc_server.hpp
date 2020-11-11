#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <memory>
#include <string>

namespace net {

class ipc_rx_connection;

class ipc_server {
public:
    ~ipc_server();

    void start(const std::string& path, int expected_backlog_size = 0);
    bool stop();
    bool is_ready() const noexcept;

    std::unique_ptr<ipc_rx_connection> process_connection();

private:
    int listen_fd {-1};
    sockaddr_un server_addr{};
    std::string server_shared_name {};
};
}
