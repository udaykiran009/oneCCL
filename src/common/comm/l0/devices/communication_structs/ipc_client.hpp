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

class ipc_tx_connection;

class ipc_client {
public:
    ~ipc_client();

    std::shared_ptr<ipc_tx_connection> create_connection(const std::string& addr);
    bool stop_all();
private:
    std::map<std::string, std::shared_ptr<ipc_tx_connection>> connections;
};
}
