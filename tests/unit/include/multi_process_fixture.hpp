#pragma once

#include <sys/types.h>
#include <unistd.h>

#include "common_platform_fixture.hpp"

namespace net {
class ipc_rx_connection;
class ipc_tx_connection;
class ipc_server;
} // namespace net
class default_fixture : public platform_fixture {
protected:
    default_fixture() : platform_fixture("") {}

    virtual ~default_fixture() {}
    void SetUp() override {
        create_global_platform();
    }
};

class multi_platform_fixture : public platform_fixture {
public:
    int wait_for_child_fork(int pid) {
        return 0;
    }

protected:
    multi_platform_fixture(
        const std::string& affinities = std::string(get_global_device_indices()));
    virtual ~multi_platform_fixture() override;

    bool is_child() const {
        return !pid;
    }

    void wait_phase(unsigned char phase);

    virtual void SetUp() override;
    virtual void TearDown() override;

    // IPC handles
    using ipc_own_ptr = std::shared_ptr<native::ccl_device::device_ipc_memory_handle>;
    using ipc_own_ptr_container = std::list<ipc_own_ptr>;

    template <class NativeType, class... Handles>
    void register_ipc_memories_data(std::shared_ptr<native::ccl_context> ctx,
                                    size_t thread_idx,
                                    Handles&&... h) {
        std::array<native::ccl_device::device_memory<NativeType>*, sizeof...(h)> memory_handles{
            h...
        };

        ipc_own_ptr_container& ipc_cont = ipc_mem_per_thread_storage[thread_idx];
        std::transform(memory_handles.begin(),
                       memory_handles.end(),
                       std::back_inserter(ipc_cont),
                       [ctx](native::ccl_device::device_memory<NativeType>* memory_handle) {
                           auto device_ptr = memory_handle->get_owner().lock();
                           return device_ptr->create_shared_ipc_memory_handle(memory_handle->handle,
                                                                              ctx);
                       });
    }

    template <class... Handles>
    void register_ipc_flags_data(std::shared_ptr<native::ccl_context> ctx,
                                 size_t thread_idx,
                                 Handles&&... h) {
        using NativeType = int;
        std::array<native::ccl_device::device_memory<NativeType>*, sizeof...(h)> memory_handles{
            h...
        };

        ipc_own_ptr_container& ipc_cont = ipc_flag_per_thread_storage[thread_idx];
        std::transform(memory_handles.begin(),
                       memory_handles.end(),
                       std::back_inserter(ipc_cont),
                       [ctx](native::ccl_device::device_memory<NativeType>* memory_handle) {
                           auto device_ptr = memory_handle->get_owner().lock();
                           return device_ptr->create_shared_ipc_memory_handle(memory_handle->handle,
                                                                              ctx);
                       });
    }
    template <class... Handles>
    void create_ipcs(std::shared_ptr<native::ccl_context> ctx,
                     size_t thread_idx,
                     size_t threads_count,
                     Handles*... h) {}
    using ipc_remote_ptr = std::shared_ptr<native::ccl_device::device_ipc_memory>;
    using ipc_remote_ptr_container = std::list<ipc_remote_ptr>;
    std::map<native::ccl_device*, ipc_remote_ptr_container> send_receive_ipc_memory(
        const std::vector<std::shared_ptr<native::ccl_device>>& devices,
        std::shared_ptr<native::ccl_context> ctx);
    std::map<native::ccl_device*, ipc_remote_ptr_container> send_receive_ipc_flags(
        const std::vector<std::shared_ptr<native::ccl_device>>& devices,
        std::shared_ptr<native::ccl_context> ctx);

    int pid = 0;
    enum { TO_CHILD, TO_PARENT };

    int pid_pair[2];
    int* other_pid = nullptr;
    int* my_pid = nullptr;

    int sockets[2];
    int communication_socket = 0;

private:
    std::unique_ptr<net::ipc_server> server;
    std::unique_ptr<net::ipc_tx_connection> tx_conn;
    std::unique_ptr<net::ipc_rx_connection> rx_conn;
    unsigned char phase_sync_count;

    std::vector<ipc_own_ptr> ipc_mem_allocated_storage;
    std::map<size_t, ipc_own_ptr_container> ipc_mem_per_thread_storage;

    std::vector<ipc_own_ptr> ipc_flag_allocated_storage;
    std::map<size_t, ipc_own_ptr_container> ipc_flag_per_thread_storage;

    template <class ipc_storage_type>
    std::map<native::ccl_device*, ipc_remote_ptr_container> send_receive_ipc_impl(
        ipc_storage_type& storage,
        const std::vector<std::shared_ptr<native::ccl_device>>& devices,
        std::shared_ptr<native::ccl_context> ctx);
};
