#include "common/global/global.hpp"
#include "common/utils/fd_info.hpp"
#include "sched/entry/ze/ze_handle_exchange_entry.hpp"
#include "sched/entry/ze/ze_primitives.hpp"
#include "sched/ze/ze_handle_manager.hpp"

#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>

static void cast_pool_to_mem_handle(ze_ipc_mem_handle_t* mem,
                                    const ze_ipc_event_pool_handle_t* pool) {
    static_assert(sizeof(ze_ipc_mem_handle_t) == sizeof(ze_ipc_event_pool_handle_t));
    memcpy(mem, pool, sizeof(*pool));
}

ze_handle_exchange_entry::ze_handle_exchange_entry(ccl_sched* sched,
                                                   ccl_comm* comm,
                                                   const std::vector<mem_desc_t>& in_buffers,
                                                   int skip_rank)
        : sched_entry(sched),
          comm(comm),
          in_buffers(in_buffers),
          rank(comm->rank()),
          comm_size(comm->size()),
          skip_rank(skip_rank) {
    LOG_DEBUG("init");

    CCL_THROW_IF_NOT(sched, "no sched");
    CCL_THROW_IF_NOT(!in_buffers.empty(), "in_buffers should be non empty");

    if (ccl::global_data::env().ze_ipc_exchange == ccl::ze::ipc_exchange_mode::sockets) {
        if (comm_size == 1) {
            this->skip_rank = rank;
        }
        poll_fds.reserve(max_pfds);
        std::string unique_tag =
            std::to_string(comm->get_comm_id()) + "-" + std::to_string(sched->sched_id) + "-" +
            std::to_string(sched->get_op_id()) + "-" + std::to_string(getuid()) + "-" +
            comm->get_topo_manager().get_uuid(0);
        right_peer_socket_name =
            "/tmp/ccl-handle-" + std::to_string((rank + 1) % comm_size) + "-" + unique_tag;
        left_peer_socket_name = "/tmp/ccl-handle-" + std::to_string(rank) + "-" + unique_tag;
    }
    create_local_ipc_handles(in_buffers);
    LOG_DEBUG("init completed");
}

ze_handle_exchange_entry::~ze_handle_exchange_entry() {
    if (ccl::global_data::env().ze_ipc_exchange == ccl::ze::ipc_exchange_mode::sockets) {
        close_sockets();
        unlink_sockets();
    }
}

void ze_handle_exchange_entry::start() {
    start_buf_idx = start_peer_idx = 0;
    skip_first_send = false;
    status = ccl_sched_entry_status_started;

    if (comm_size == 1) {
        status = ccl_sched_entry_status_complete;
    }
}

void ze_handle_exchange_entry::create_local_ipc_handles(const std::vector<mem_desc_t>& bufs) {
    if (comm_size == 1) {
        this->skip_rank = rank;
    }

    handles.resize(comm_size);
    for (auto& buffers : handles) {
        buffers.resize(in_buffers.size());
    }
    LOG_DEBUG("handles size: ", handles.size(), ", in_buffers size: ", in_buffers.size());

    for (size_t buf_idx = 0; buf_idx < in_buffers.size(); buf_idx++) {
        int mem_handle = ccl::utils::invalid_mem_handle;
        auto mem_ptr = in_buffers[buf_idx].first;
        CCL_THROW_IF_NOT(mem_ptr, "memory pointer is nullptr");
        auto mem_type = in_buffers[buf_idx].second;
        mem_info_t mem_info{};

        ze_ipc_mem_handle_t ipc_handle{};
        if (rank != this->skip_rank) {
            if (mem_type == ccl::ze::ipc_mem_type::memory) {
                // zeMemGetIpcHandle requires the provided pointer to be the base of an allocation.
                // We handle this the following way: for an input buffer retrieve its base pointer
                // and the offset from this base ptr. The base ptr is used for zeMemGetIpcHandle
                // and the offset is sent to the other rank. On that rank the base ptr is retrieved
                // and offsetted to get the actual input buffer ptr.
                mem_info = get_mem_info(mem_ptr);
                sched->get_memory().handle_manager.get_handle(mem_info.first, &ipc_handle);

                if (ccl::global_data::env().ze_ipc_exchange == ccl::ze::ipc_exchange_mode::drmfd) {
                    device_fds = ccl::global_data::get().ze_data->fd_manager->get_device_fds();
                    mem_handle = ipc_to_mem_handle(
                        ipc_handle,
                        ccl::ze::get_parent_device_id(sched->coll_param.stream->get_ze_device()));
                }
            }
            else if (mem_type == ccl::ze::ipc_mem_type::pool) {
                ze_ipc_event_pool_handle_t pool_handle;
                sched->get_memory().handle_manager.get_handle(
                    static_cast<ze_event_pool_handle_t>(mem_ptr), &pool_handle);
                // since ze_ipc_event_pool_handle_t and ze_ipc_mem_handle_t are similar,
                // we cast ze_ipc_event_pool_handle_t to ze_ipc_mem_handle_t, but
                // maybe this is not the most correct way
                cast_pool_to_mem_handle(&ipc_handle, &pool_handle);
            }
            else {
                CCL_THROW("unknown memory type");
            }
        }

        handles[rank][buf_idx] = { ipc_handle, mem_info.second, mem_type, mem_handle };
        LOG_DEBUG("set IPC handle: { rank: ",
                  rank,
                  ", buf_idx: ",
                  buf_idx,
                  ", mem_type: ",
                  to_string(mem_type),
                  ", mem_handle: ",
                  mem_handle,
                  " }");
    }
}

int ze_handle_exchange_entry::ipc_to_mem_handle(const ze_ipc_mem_handle_t& ipc_handle,
                                                const int parent_dev_id) {
    int dmabuf_fd;
    // convert dma_buf fd to GEM handle
    memcpy(&dmabuf_fd, &ipc_handle, sizeof(dmabuf_fd));

    int mem_handle = ccl::ze::fd_manager::fd_to_mem_handle(device_fds[parent_dev_id], dmabuf_fd);
    CCL_THROW_IF_NOT(mem_handle != ccl::utils::invalid_mem_handle,
                     "fd_to_mem_handle: ivalid mem_handle: ",
                     mem_handle);
    LOG_DEBUG("device_fd: ", device_fds[parent_dev_id], ", mem_handle: ", mem_handle);
    return mem_handle;
}

ze_ipc_mem_handle_t ze_handle_exchange_entry::mem_to_ipc_handle(const int& mem_handle,
                                                                const int parent_dev_id) {
    int fd = ccl::ze::fd_manager::mem_handle_to_fd(device_fds[parent_dev_id], mem_handle);
    CCL_THROW_IF_NOT(fd != ccl::utils::invalid_fd, "mem_handle_to_fd: invalid fd: ", fd);
    LOG_DEBUG("device_fd: ", device_fds[parent_dev_id], ", mem_handle: ", mem_handle, ", fd: ", fd);
    return ccl::ze::get_handle_from_fd(fd);
}

void ze_handle_exchange_entry::fill_payload(payload_t& payload,
                                            const std::vector<mem_desc_t>& bufs,
                                            size_t buf_idx) {
    payload.mem_handle = handles[rank][buf_idx].mem_handle;
    payload.mem_type = handles[rank][buf_idx].mem_type;
    payload.mem_offset = handles[rank][buf_idx].mem_offset;
    payload.remote_pid = getpid();
    const void* ptr = bufs[buf_idx].first;

    ze_context_handle_t remote_context{};
    ze_device_handle_t remote_device{};
    ze_memory_allocation_properties_t mem_alloc_props;
    if (!ccl::ze::get_buffer_context_and_device(
            ptr, &remote_context, &remote_device, &mem_alloc_props)) {
        CCL_THROW("unable to get context from ptr\n");
    }
    ssize_t remote_context_id{ ccl::utils::invalid_context_id };
    if (!ccl::ze::get_context_global_id(remote_context, &remote_context_id)) {
        CCL_THROW("unable to get global id for context\n");
    }
    ssize_t remote_device_id{ ccl::utils::invalid_device_id };
    if (!ccl::ze::get_device_global_id(remote_device, &remote_device_id)) {
        CCL_THROW("unable to get global id for device\n");
    }

    payload.remote_mem_alloc_id = mem_alloc_props.id;
    payload.remote_context_id = remote_context_id;
    payload.remote_device_id = remote_device_id;
}

void ze_handle_exchange_entry::fill_remote_handle(const payload_t& payload,
                                                  ze_ipc_mem_handle_t ipc_handle,
                                                  size_t idx,
                                                  size_t buf_idx) {
    handles[idx][buf_idx] = {
        ipc_handle, payload.mem_offset, payload.mem_type, payload.mem_handle
    };
    handles[idx][buf_idx].remote_mem_alloc_id = payload.remote_mem_alloc_id;
    handles[idx][buf_idx].remote_context_id = payload.remote_context_id;
    handles[idx][buf_idx].remote_pid = payload.remote_pid;
    handles[idx][buf_idx].remote_device_id = payload.remote_device_id;
    LOG_DEBUG("get IPC handle: { peer: ",
              idx,
              ", buf_idx: ",
              buf_idx,
              ", mem_type: ",
              to_string(payload.mem_type),
              " }");
}

int ze_handle_exchange_entry::drmfd_mode_exchange(const std::vector<mem_desc_t>& bufs) {
    for (size_t buf_idx = 0; buf_idx < bufs.size(); buf_idx++) {
        std::vector<payload_t> payloads(comm_size);
        payload_t payload{};
        fill_payload(payload, bufs, buf_idx);

        if (!(ccl::utils::allgather(
                comm->get_atl_comm(), &payload, payloads.data(), sizeof(payload_t)))) {
            CCL_THROW("allgather exchange is failed");
        }

        for (size_t idx = 0; idx < payloads.size(); idx++) {
            if (comm->rank() == (int)idx) {
                continue;
            }
            auto parent_dev_id =
                ccl::global_data::get().ze_data->devices[payloads[idx].remote_device_id].parent_idx;
            fill_remote_handle(payloads[idx],
                               mem_to_ipc_handle(payloads[idx].mem_handle, parent_dev_id),
                               idx,
                               buf_idx);
        }
    }

    LOG_DEBUG("handles size: ", handles.size(), ", in_buffers size: ", bufs.size());
    sched->get_memory().handle_manager.set(handles);
    return 0;
}

int ze_handle_exchange_entry::sockets_mode_exchange(const std::vector<mem_desc_t>& bufs) {
    if (!is_created) {
        // server
        left_peer_connect_socket =
            create_server_socket(left_peer_socket_name, &left_peer_addr, &left_peer_addr_len);

        // client
        right_peer_socket =
            create_client_socket(right_peer_socket_name, &right_peer_addr, &right_peer_addr_len);

        is_created = true;
    }

    if (!is_connected) {
        if (connect_call(
                right_peer_socket, &right_peer_addr, right_peer_addr_len, right_peer_socket_name)) {
            return ccl::utils::invalid_err_code;
        }
        is_connected = true;
    }

    if (!is_accepted) {
        if (accept_call(left_peer_connect_socket,
                        &left_peer_addr,
                        &left_peer_addr_len,
                        left_peer_socket_name,
                        left_peer_socket)) {
            return ccl::utils::invalid_err_code;
        }

        struct pollfd poll_fd {};
        poll_fd.fd = left_peer_socket;
        poll_fd.events = POLLIN;
        poll_fd.revents = 0;
        poll_fds.push_back(poll_fd);

        is_accepted = true;
    }

    CCL_THROW_IF_NOT(poll_fds.size() == 1, "unexpected poll_fds size: ", poll_fds.size());

    for (size_t buf_idx = start_buf_idx; buf_idx < in_buffers.size(); buf_idx++) {
        for (int peer_idx = start_peer_idx; peer_idx < comm_size - 1; peer_idx++) {
            int peer = (comm_size + rank - 1 - peer_idx) % comm_size;

            if ((peer_idx == 0) && !skip_first_send && (rank != this->skip_rank)) {
                // send own handle to right peer
                int send_fd = ccl::ze::get_fd_from_handle(handles[rank][buf_idx].ipc_handle);
                payload_t payload{};
                fill_payload(payload, in_buffers, buf_idx);
                ccl::utils::sendmsg_call(
                    right_peer_socket, send_fd, &payload, sizeof(payload_t), rank);
                skip_first_send = true;
            }

            if (peer == this->skip_rank)
                continue;

            int poll_ret = poll(&poll_fds[0], poll_fds.size(), timeout_ms);

            if (poll_ret == poll_expire_err_code) {
                return ccl::utils::invalid_err_code;
            }
            else if (poll_ret == POLL_ERR) {
                CCL_THROW("poll: error: ", strerror(errno), ", ret: ", poll_ret);
            }

            CCL_THROW_IF_NOT(poll_ret > 0, "unexpected poll ret: ", poll_ret);

            if (poll_fds[0].revents & POLLIN) {
                int recv_fd = ccl::utils::invalid_fd;

                // recv data from left peer
                payload_t payload{};
                ccl::utils::recvmsg_call(
                    left_peer_socket, &recv_fd, &payload, sizeof(payload_t), rank);

                ze_ipc_mem_handle_t tmp_handle = ccl::ze::get_handle_from_fd(recv_fd);

                // we don't know anything about the memory type on the other side,
                // so we take it from our list. This assumes that the lists of types (exactly types)
                // on the sending and receiving side are the same in both value and quantity
                fill_remote_handle(payload, tmp_handle, peer, buf_idx);

                if (peer_idx < (comm_size - 2)) {
                    // proxy data to right peer
                    ccl::utils::sendmsg_call(
                        right_peer_socket, recv_fd, &payload, sizeof(payload), rank);
                }
                start_peer_idx++;
            }
            else if (poll_fds[0].revents & POLLERR) {
                CCL_THROW("poll: POLLERR, buf_idx: ", buf_idx, ", peer_idx ", peer_idx);
            }
            else if (poll_fds[0].revents & POLLHUP) {
                CCL_THROW("poll: POLLHUP, buf_idx: ", buf_idx, ", peer_idx ", peer_idx);
            }
            else {
                LOG_TRACE("poll: nothing to receive, buf_idx: ", buf_idx, ", peer_idx ", peer_idx);
                // nothing to receive
                // continue with the same buf_idx/peer_idx in the next update() call
                return ccl::utils::invalid_err_code;
            }
        }
        start_peer_idx = 0;
        skip_first_send = false;
        start_buf_idx++;
    }

    LOG_DEBUG("handles size: ", handles.size(), ", in_buffers size: ", in_buffers.size());

    sched->get_memory().handle_manager.set(handles);

    if (ccl::global_data::env().enable_close_fd_wa) {
        // in order to avoid possibility of reaching an opened fd limit in case of async execution
        // (when a user submits multiple operations and waits for them keeping the corresponding
        // ccl events and master scheds alive), we need to close sockets as soon as we can instead
        // of waiting till destruction of the entry.
        close_sockets();
    }
    return 0;
}

void ze_handle_exchange_entry::update() {
    if (ccl::global_data::env().ze_ipc_exchange == ccl::ze::ipc_exchange_mode::sockets) {
        if (sockets_mode_exchange(in_buffers)) {
            return;
        }
    }
    else if (ccl::global_data::env().ze_ipc_exchange == ccl::ze::ipc_exchange_mode::drmfd) {
        if (drmfd_mode_exchange(in_buffers)) {
            return;
        }
    }
    else {
        CCL_THROW("unexpected ipc_exchange_mode");
    }

    status = ccl_sched_entry_status_complete;
    LOG_DEBUG("completed: ", name());
}

int ze_handle_exchange_entry::create_server_socket(const std::string& socket_name,
                                                   struct sockaddr_un* socket_addr,
                                                   int* addr_len) {
    int ret = 0;
    memset(&(*socket_addr), 0, sizeof((*socket_addr)));

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        unlink_sockets();

        CCL_THROW("cannot create a server socket: ",
                  sock,
                  ", errno: ",
                  strerror(errno),
                  ", socket_name: ",
                  socket_name,
                  ", ",
                  to_string(ccl::utils::get_fd_info()));
    }

    socket_addr->sun_family = AF_UNIX;
    strncpy(socket_addr->sun_path, socket_name.c_str(), sizeof(socket_addr->sun_path) - 1);
    socket_addr->sun_path[sizeof(socket_addr->sun_path) - 1] = '\0';
    *addr_len = sizeof((*socket_addr));

    ret = fcntl(sock, F_SETFL, O_NONBLOCK);
    CCL_THROW_IF_NOT(
        !ret, "fcntl error: ", ret, ", errno: ", strerror(errno), ", socket_name: ", socket_name);

    unlink(socket_name.c_str());

    ret = bind(sock, ((struct sockaddr*)&(*socket_addr)), *addr_len);
    CCL_THROW_IF_NOT(
        !ret, "bind error: ", ret, ", errno: ", strerror(errno), ", socket_name: ", socket_name);

    ret = listen(sock, comm_size);
    CCL_THROW_IF_NOT(
        !ret, "listen error: ", ret, ", errno: ", strerror(errno), ", socket_name: ", socket_name);

    return sock;
}

int ze_handle_exchange_entry::create_client_socket(const std::string& socket_name,
                                                   struct sockaddr_un* socket_addr,
                                                   int* addr_len) {
    memset(&(*socket_addr), 0, sizeof(*(socket_addr)));

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    CCL_THROW_IF_NOT(sock >= 0,
                     "cannot create a client socket: ",
                     sock,
                     ", errno: ",
                     strerror(errno),
                     ", ",
                     to_string(ccl::utils::get_fd_info()));

    socket_addr->sun_family = AF_UNIX;
    strncpy(socket_addr->sun_path, socket_name.c_str(), sizeof(socket_addr->sun_path) - 1);
    socket_addr->sun_path[sizeof(socket_addr->sun_path) - 1] = '\0';
    *addr_len = sizeof((*socket_addr));

    return sock;
}

int ze_handle_exchange_entry::accept_call(int connect_socket,
                                          struct sockaddr_un* socket_addr,
                                          int* addr_len,
                                          const std::string& socket_name,
                                          int& sock) {
    sock = accept(connect_socket, ((struct sockaddr*)&(*socket_addr)), ((socklen_t*)&(*addr_len)));
    if (sock < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            LOG_TRACE("accept eagain: ", strerror(errno), ", socket_name: ", socket_name);
            return errno;
        }

        CCL_THROW("accept error: ",
                  strerror(errno),
                  " sock: ",
                  sock,
                  ", socket_name: ",
                  socket_name,
                  ", ",
                  to_string(ccl::utils::get_fd_info()));
    }

    LOG_DEBUG("accept from [", comm->rank(), "] (wait) on: ", socket_name);
    return 0;
}

int ze_handle_exchange_entry::connect_call(int sock,
                                           struct sockaddr_un* socket_addr,
                                           int addr_len,
                                           const std::string& socket_name) {
    int ret = connect(sock, ((struct sockaddr*)&(*socket_addr)), addr_len);
    if (ret < 0) {
        if (errno == ECONNREFUSED || errno == ENOENT) {
            return errno;
        }
        CCL_THROW(
            "connect error: ", ret, ", errno: ", strerror(errno), ", socket_name: ", socket_name);
    }

    LOG_DEBUG("connect from: [",
              comm->rank(),
              "] to [",
              (comm->rank() - 1 + comm->size()) % comm->size(),
              "] with: ",
              socket_name);

    return 0;
}

ze_handle_exchange_entry::mem_info_t ze_handle_exchange_entry::get_mem_info(const void* ptr) {
    void* base_ptr{};
    size_t alloc_size{};
    sched->get_memory().handle_manager.get_address_range(ptr, &base_ptr, &alloc_size);
    return { base_ptr, static_cast<const char*>(ptr) - static_cast<char*>(base_ptr) };
}

void ze_handle_exchange_entry::unlink_sockets() {
    unlink(left_peer_socket_name.c_str());
}

void ze_handle_exchange_entry::close_sockets() {
    // use the flag to protect from double close: right now we close sockets
    // in update() but this close is under env variable, so we need to close
    // the socket during entry's destruction, to make sure we don't have any
    // open sockets left.
    if (!sockets_closed) {
        close(left_peer_connect_socket);
        close(left_peer_socket);
        close(right_peer_socket);
        sockets_closed = true;
    }
}

void ze_handle_exchange_entry::dump_detail(std::stringstream& str) const {
    ccl_logger::format(str,
                       "comm ",
                       comm->to_string(),
                       ", right_peer ",
                       right_peer_socket_name,
                       ", left_peer ",
                       left_peer_socket_name,
                       ", in_buffers size ",
                       in_buffers.size(),
                       ", handles size ",
                       handles.size(),
                       "\n");
}
