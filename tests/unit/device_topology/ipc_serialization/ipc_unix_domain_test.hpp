#pragma once
#include "ipc_fixture.hpp"
#include "common/comm/l0/devices/communication_structs/ipc_server.hpp"
#include "common/comm/l0/devices/communication_structs/ipc_connection.hpp"

namespace ipc_unix_suite {

using native_type = int;

constexpr size_t connect_iteration = 100;

TEST_F(ipc_handles_fixture, ipc_unix_server_test) {
    using namespace native;
    using namespace net;

    ipc_server server;
    std::string addr{ "ipc_unix_suite_" };
    std::string my_addr = addr + std::to_string(*my_pid);
    std::string other_addr = addr + std::to_string(*other_pid);

    // start server
    output << "PID: " << *my_pid << " start server: " << my_addr << std::endl;
    try {
        server.start(my_addr);
    }
    catch (const std::exception& ex) {
        UT_ASSERT(false, "Cannot start IPC server on: " << my_addr << ", error: " << ex.what());
    }

    // test rx & tx conenction
    unsigned char phase = 0;
    for (size_t connection_index = 0; connection_index < connect_iteration; connection_index++) {
        // initiate connection
        wait_phase(phase++);

        output << "PID: " << *my_pid << ", phase: " << (int)phase
               << " connect to server: " << other_addr << std::endl;
        std::unique_ptr<ipc_tx_connection> tx_conn;
        try {
            tx_conn.reset(new ipc_tx_connection(other_addr));
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false,
                      "Cannot create TX connection to other IPC server on: "
                          << other_addr << ", error: " << ex.what());
        }

        // wait incoming connection
        wait_phase(phase++);

        output << "PID: " << *my_pid << ", phase: " << (int)phase
               << " accept conn from my server: " << my_addr << std::endl;
        std::unique_ptr<ipc_rx_connection> rx_conn;
        try {
            rx_conn = server.process_connection();
            UT_ASSERT(rx_conn, "RX connection have to be accepted at moment");
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false,
                      "Cannot accept RX connectio from my IPC server on: " << my_addr << ", error: "
                                                                           << ex.what());
        }

        // send data
        wait_phase(phase++);
        std::string tx_send = my_addr + "|" + std::to_string(connection_index);
        output << "PID: " << *my_pid << ", phase: " << (int)phase << " send data: " << tx_send
               << " to server: " << other_addr << std::endl;
        std::vector<uint8_t> tx_data(tx_send.size(), 0);
        memcpy(tx_data.data(), tx_send.data(), tx_send.size());
        try {
            tx_conn->send_msg_with_pid_data(tx_data, {}, 0);
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false,
                      "Cannot send TX data to other IPC server on: " << other_addr
                                                                     << ", error: " << ex.what());
        }

        // receive data
        wait_phase(phase++);
        std::vector<uint8_t> rx_data(tx_send.size(), 0);
        std::vector<connection::fd_t> out_pids_resized;
        try {
            rx_conn->recv_msg_with_pid_data(rx_data, out_pids_resized, 0);
        }
        catch (const std::exception& ex) {
            UT_ASSERT(
                false,
                "Cannot recv RX data on my IPC server on: " << my_addr << ", error: " << ex.what());
        }
        std::string rx_recv(reinterpret_cast<const char*>(rx_data.data()), rx_data.size());
        output << "PID: " << *my_pid << ", phase: " << (int)phase << " received data: " << rx_recv
               << " from server: " << my_addr << std::endl;

        std::string checked_rx_recv = other_addr + "|" + std::to_string(connection_index);
        UT_ASSERT(checked_rx_recv == rx_recv, "Unexpected data received");
    }

    wait_phase(phase++);

    // stop server
    bool stopped = server.stop();
    UT_ASSERT(stopped, "Stop server failed");
    stopped = server.stop();
    UT_ASSERT(!stopped, "Duliacted Stop server failed");
    output << "PID: " << *my_pid << ", phase: " << (int)phase << ", servers have stopped"
           << std::endl;

    // test rx & tx on stopped servers
    for (size_t connection_index = 0; connection_index < connect_iteration; connection_index++) {
        // initiate connection
        wait_phase(phase++);

        output << "PID: " << *my_pid << ", phase: " << (int)phase
               << " connect to server: " << other_addr << std::endl;
        std::unique_ptr<ipc_tx_connection> tx_conn;
        bool dropped = false;
        try {
            tx_conn.reset(new ipc_tx_connection(other_addr));
        }
        catch (const std::exception& ex) {
            dropped = true;
        }
        UT_ASSERT(dropped,
                  "Impossible create TX connection to other IPC server on: " << other_addr);

        // wait incoming connection
        wait_phase(phase++);

        output << "PID: " << *my_pid << ", phase: " << (int)phase
               << " accept conn from my server: " << my_addr << std::endl;
        std::unique_ptr<ipc_rx_connection> rx_conn;
        try {
            dropped = false;
            rx_conn = server.process_connection();
            UT_ASSERT(!rx_conn, "RX connection have to be accepted at moment");
        }
        catch (const std::exception& ex) {
            dropped = true;
        }
        UT_ASSERT(dropped, "Impossible accept RX connectio from my IPC server on: " << my_addr);
    }
}

} // namespace ipc_unix_suite
