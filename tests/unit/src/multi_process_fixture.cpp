#include "multi_process_fixture.hpp"
#include "common/comm/l0/devices/communication_structs/ipc_server.hpp"
#include "common/comm/l0/devices/communication_structs/ipc_connection.hpp"

multi_platform_fixture::multi_platform_fixture(const std::string& affinities)
        : platform_fixture(affinities) {}

multi_platform_fixture::~multi_platform_fixture() {}

void multi_platform_fixture::wait_phase(unsigned char phase) {
    int ret = utils::writeToSocket(communication_socket, &phase, sizeof(phase));
    if (ret != 0) {
        std::cerr << "Cannot writeToSocket on phase: " << (int)phase
                  << ", error: " << strerror(errno) << std::endl;
        abort();
    }

    unsigned char get_phase = phase + 1;
    ret = utils::readFromSocket(communication_socket, &get_phase, sizeof(get_phase));
    if (ret != 0) {
        std::cerr << "Cannot readFromSocket on phase: " << (int)phase
                  << ", error: " << strerror(errno) << std::endl;
        abort();
    }

    if (phase != get_phase) {
        std::cerr << "wait_phase phases mismatch! wait phase: " << (int)phase
                  << ", got phase: " << (int)get_phase << std::endl;
        abort();
    }
}

void multi_platform_fixture::SetUp() {
    if (cluster_device_indices.size() != 2) {
        std::cerr << __FUNCTION__ << " - not enough devices in \"" << ut_device_affinity_mask_name
                  << "\"."
                  << "Two devices required at least" << std::endl;
        abort();
    }
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sockets) < 0) {
        std::cerr << __FUNCTION__
                  << " - cannot create sockets pairt before fork: " << strerror(errno) << std::endl;
        abort();
    }

    pid_pair[TO_CHILD] = getpid();
    pid_pair[TO_PARENT] = getpid();

    std::string ipc_listening_addr{ "multi_platform_fixture_" };
    std::string ipc_connecting_addr{ "multi_platform_fixture_" };

    pid = fork();
    if (pid < 0) {
        std::cerr << __FUNCTION__ << " - cannot fork() process: " << strerror(errno) << std::endl;
        abort();
    }
    else if (pid == 0) //child
    {
        close(sockets[TO_CHILD]);
        pid_pair[TO_CHILD] = getpid();
        my_pid = &pid_pair[TO_CHILD];
        other_pid = &pid_pair[TO_PARENT];

        communication_socket = sockets[TO_PARENT];

        // create local to child process driver & devices
        platform_fixture::SetUp();
        const auto& local_affinity = cluster_device_indices[TO_CHILD];
        create_local_platform(local_affinity);
    }
    else {
        close(sockets[TO_PARENT]);

        pid_pair[TO_CHILD] = pid;

        my_pid = &pid_pair[TO_PARENT];
        other_pid = &pid_pair[TO_CHILD];

        communication_socket = sockets[TO_CHILD];

        // create local to parent process driver & devices
        platform_fixture::SetUp();
        const auto& local_affinity = cluster_device_indices[TO_PARENT];
        create_local_platform(local_affinity);
    }

    // start IPC server
    std::string my_addr = ipc_listening_addr + std::to_string(*my_pid);
    std::string remote_addr = ipc_connecting_addr + std::to_string(*other_pid);
    output << "PID: " << *my_pid << " start server: " << my_addr << std::endl;
    server.reset(new net::ipc_server);
    server->start(my_addr);

    // etablish connection to remote IPC server
    this->wait_phase(phase_sync_count++);

    output << "PID: " << *my_pid << " establish connection to: " << remote_addr << std::endl;
    tx_conn.reset(new net::ipc_tx_connection(remote_addr));

    // wait incoming connection
    this->wait_phase(phase_sync_count++);

    output << "PID: " << *my_pid << " accept connection from: " << remote_addr << std::endl;
    rx_conn = server->process_connection();
    UT_ASSERT(rx_conn, "RX connection have to be accepted at moment");
}

void multi_platform_fixture::TearDown() {
    //TODO waitpid for parent
    if (pid) {
        std::cout << "PID: " << *my_pid << " wait child: " << *other_pid << std::endl;
        int status = 0;
        int ret = waitpid(pid, &status, 0);
        UT_ASSERT(ret == pid, "Waitpid return fails");
    }
    else {
        std::cout << "PID: " << *my_pid << " exit" << std::endl;
        quick_exit(0);
    }
}

template <class ipc_storage_type>
std::map<native::ccl_device*, multi_platform_fixture::ipc_remote_ptr_container>
multi_platform_fixture::send_receive_ipc_impl(
    ipc_storage_type& storage,
    const std::vector<std::shared_ptr<native::ccl_device>>& devices,
    std::shared_ptr<native::ccl_context> ctx) {
    using namespace native;

    // Convert for TX/RX format and send handles
    using ipc_handle_container = std::vector<ccl_device::device_ipc_memory_handle>;
    using ipc_handles_ptr_container =
        std::vector<std::shared_ptr<ccl_device::device_ipc_memory_handle>>;

    std::map<ccl_device*, ipc_handle_container> ipc_mem_storage_to_send;
    std::map<ccl_device*, ipc_handles_ptr_container> ipc_mem_storage_to_receive;

    std::map<ccl_device*, ipc_handle_container> ipc_flags_storage_to_send;
    std::map<ccl_device*, ipc_handles_ptr_container> ipc_flags_storage_to_receive;
    size_t thread_id = 0;
    for (auto device : devices) {
        ccl_device* device_ptr = device.get();

        // collect IPC memory
        try {
            auto& memory_handles = storage[thread_id];

            // prepare send handles and receive handles storage
            ipc_handle_container& out_ipc_handles = ipc_mem_storage_to_send[device_ptr];
            out_ipc_handles.reserve(memory_handles.size());

            ipc_handles_ptr_container& in_ipc_handles = ipc_mem_storage_to_receive[device_ptr];
            in_ipc_handles.reserve(memory_handles.size());

            // create ic handles to send it later
            std::transform(
                memory_handles.begin(),
                memory_handles.end(),
                std::back_inserter(out_ipc_handles),
                [](std::shared_ptr<ccl_device::device_ipc_memory_handle>& memory_handle_ptr) {
                    return std::move(*memory_handle_ptr);
                });
        }
        catch (const std::exception& ex) {
            std::stringstream ss;
            ss << "Cannot create IPC memory handle for device: " << device_ptr->to_string()
               << "\nError: " << ex.what();
            throw std::runtime_error(ss.str());
        }
        thread_id++;
    }

    // send to relative process
    std::map<ccl_device*, std::vector<uint8_t>> received_raw_data_per_device;

    for (const auto& device_storage : ipc_mem_storage_to_send) {
        const ipc_handle_container& ipc_mem_handles = device_storage.second;
        std::vector<uint8_t> serialized_raw_handles =
            tx_conn->send_ipc_memory(ipc_mem_handles, *this->my_pid);

        //prepare recv array
        received_raw_data_per_device[device_storage.first].resize(serialized_raw_handles.size());
    }

    // receive MEM data
    std::map<ccl_device*, ipc_remote_ptr_container> ipc_client_memory;

    thread_id = 0;
    for (auto& device_storage : ipc_mem_storage_to_receive) {
        ipc_handles_ptr_container& ipc_handles = device_storage.second;
        std::vector<uint8_t>& data_to_recv = received_raw_data_per_device[device_storage.first];

        size_t received_rank = 0;
        auto got = rx_conn->receive_ipc_memory(data_to_recv, received_rank);
        ipc_handles.swap(got);

        if (static_cast<int>(received_rank) != *this->other_pid) {
            throw std::runtime_error("Received rank is invalid");
        }

        // recover handles
        const auto& global_devices_array = get_global_devices();
        size_t index = 0;
        for (auto& recv_ipc_handle : ipc_handles) {
            std::shared_ptr<ccl_device> owner_device = recv_ipc_handle->get_owner().lock();
            auto ipc_device_it =
                std::find_if(global_devices_array.begin(),
                             global_devices_array.end(),
                             [owner_device](const std::shared_ptr<ccl_device>& dev) {
                                 return dev->handle == owner_device->handle;
                             });
            if (ipc_device_it == global_devices_array.end()) {
                throw std::runtime_error("Cannot find ipc device in global driver");
            }

            this->output << "PID: " << *this->my_pid << " restore MEM from IPC handle" << std::endl;
            try {
                std::shared_ptr<ccl_device::device_ipc_memory> recovered_mem =
                    owner_device->restore_shared_ipc_memory(std::move(recv_ipc_handle), ctx);

                ipc_client_memory[device_storage.first].push_back(recovered_mem);
            }
            catch (const std::exception& ex) {
                std::stringstream ss;
                ss << "Cannot restore IPC memory handle at index: " << index
                   << ". Error: " << ex.what();
                throw std::runtime_error(ss.str());
            }
            index++;
        }
    }
    return ipc_client_memory;
}

std::map<native::ccl_device*, multi_platform_fixture::ipc_remote_ptr_container>
multi_platform_fixture::send_receive_ipc_memory(
    const std::vector<std::shared_ptr<native::ccl_device>>& devices,
    std::shared_ptr<native::ccl_context> ctx) {
    return send_receive_ipc_impl(ipc_mem_per_thread_storage, devices, ctx);
}

std::map<native::ccl_device*, multi_platform_fixture::ipc_remote_ptr_container>
multi_platform_fixture::send_receive_ipc_flags(
    const std::vector<std::shared_ptr<native::ccl_device>>& devices,
    std::shared_ptr<native::ccl_context> ctx) {
    return send_receive_ipc_impl(ipc_mem_per_thread_storage, devices, ctx);
}
