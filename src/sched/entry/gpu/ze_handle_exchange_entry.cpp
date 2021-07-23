#include "sched/queue/queue.hpp"
#include "ze_handle_exchange_entry.hpp"

#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)

ze_handle_exchange_entry::ze_handle_exchange_entry(ccl_sched* sched,
                                                   ccl_comm* comm,
                                                   const std::vector<void*>& in_buffers)
        : sched_entry(sched),
          comm(comm),
          in_buffers(in_buffers),
          rank(comm->rank()),
          comm_size(comm->size()),
          start_buf_idx(0),
          start_peer_idx(0),
          is_created(false),
          is_connected(false),
          is_accepted(false) {
    CCL_THROW_IF_NOT(!in_buffers.empty(), "in_buffers should be non empty");

    poll_fds.reserve(max_pfds);

    handles.resize(comm_size);

    for (size_t idx = 0; idx < comm_size; idx++) {
        handles[idx].resize(in_buffers.size());
    }
    LOG_DEBUG("handles size: ", handles.size(), ", in_buffers size: ", in_buffers.size());

    CCL_THROW_IF_NOT(sched->coll_param.stream, "null stream in ze_handle_exchange_entry");

    LOG_DEBUG("getting a native stream");
    auto native_stream = sched->coll_param.stream->get_native_stream();
    if (native_stream.get_backend() != sycl::backend::level_zero) {
        CCL_THROW("unsupported sycl backend");
    }

    auto sycl_context = native_stream.get_context();
    context = sycl_context.template get_native<sycl::backend::level_zero>();

    for (size_t buf_idx = 0; buf_idx < in_buffers.size(); buf_idx++) {
        handles.at(rank).resize(in_buffers.size());
        ze_ipc_mem_handle_t handle;

        // zeMemGetIpcHandle requires the provided pointer to be the base of an allocation.
        // We handle this the following way: for an input buffer retrieve its base pointer
        // and the offset from this base ptr. The base ptr is used for zeMemGetIpcHandle
        // and the offset is sent to the other rank. On that rank the base ptr is retrieved
        // and offsetted to get the actual input buffer ptr.
        auto mem_info = get_mem_info(context, in_buffers[buf_idx]);
        get_handle(context, mem_info.first, &handle);

        handles[rank][buf_idx] = { handle, mem_info.second };
    }

    std::ostringstream right_peer_ss;
    std::ostringstream left_peer_ss;
    std::ostringstream unique_tag_ss;

    int op_id = sched->get_op_id();
    unique_tag_ss << sched->get_comm_id() << "-" << sched->sched_id << "-" << op_id;
    right_peer_ss << "/tmp/ccl-handle-" << ((rank + 1)) % comm_size << "-" << unique_tag_ss.str();
    left_peer_ss << "/tmp/ccl-handle-" << rank << "-" << unique_tag_ss.str();

    // This is a temporary workaround around to provide uniqueness of socket files created
    // in /tmp folder, otherwise this could result in issues in case of parallel runs
    // by a single/multiple users.
    // Ideally we should use process pid for this, but right now we don't have this information
    // available for all the processes, so use this env variable instead. This works with mpiexec
    // only(this is why it's the workaround rather than a complete solution)
    static const char* mpi_uuid = getenv("I_MPI_HYDRA_UUID");
    if (mpi_uuid) {
        right_peer_ss << "-" << mpi_uuid;
        left_peer_ss << "-" << mpi_uuid;
    }

    right_peer_socket_name = right_peer_ss.str();
    left_peer_socket_name = left_peer_ss.str();

    LOG_DEBUG("started: ", name());
}

ze_handle_exchange_entry::~ze_handle_exchange_entry() {
    close_sockets();
    unlink_sockets();
}

void ze_handle_exchange_entry::start() {
    LOG_DEBUG("started: ", name());
    start_buf_idx = start_peer_idx = 0;
    status = ccl_sched_entry_status_started;
}

void ze_handle_exchange_entry::update() {
    if (!is_created) {
        // server
        left_peer_connect_socket = create_server_socket(
            left_peer_socket_name, &left_peer_addr, &left_peer_addr_len, comm_size);

        // client
        right_peer_socket =
            create_client_socket(right_peer_socket_name, &right_peer_addr, &right_peer_addr_len);

        is_created = true;
    }

    if (!is_connected) {
        if (connect_call(
                right_peer_socket, &right_peer_addr, right_peer_addr_len, right_peer_socket_name)) {
            return;
        }
        is_connected = true;
    }

    if (!is_accepted) {
        if (accept_call(left_peer_connect_socket,
                        &left_peer_addr,
                        &left_peer_addr_len,
                        left_peer_socket_name,
                        left_peer_socket)) {
            return;
        }

        struct pollfd poll_fd = { 0 };
        poll_fd.fd = left_peer_socket;
        poll_fd.events = POLLIN;
        poll_fd.revents = 0;
        poll_fds.push_back(poll_fd);

        is_accepted = true;
    }

    for (size_t buf_idx = start_buf_idx; buf_idx < in_buffers.size(); buf_idx++) {
        for (int peer_idx = start_peer_idx; peer_idx < comm_size - 1; peer_idx++) {
            int peer = (rank + 1 + peer_idx) % comm_size;

            if (peer_idx == 0) {
                int send_fd = 0;
                // send own handle to right peer
                get_fd_from_handle(&(handles[rank][buf_idx].handle), &send_fd);
                sendmsg_call(right_peer_socket, send_fd, handles[rank][buf_idx].mem_offset);
            }

            int poll_ret = poll(&poll_fds[0], poll_fds.size(), timeout_ms);
            if (poll_ret == poll_expire_err_code) {
                LOG_DEBUG("poll: timeout is expired");
                return;
            }
            else if (poll_ret == POLL_ERR) {
                CCL_THROW("poll: error: ", std::to_string(poll_ret));
            }

            if (poll_fds[0].revents & POLLIN) {
                int recv_fd = 0;
                ze_ipc_mem_handle_t tmp_handle;

                size_t mem_offset = 0;
                // recv data from left peer
                recvmsg_call(left_peer_socket, recv_fd, mem_offset);

                // invoke get_handle_from_fd to store the handle
                get_handle_from_fd(&recv_fd, &tmp_handle);
                handles[peer][buf_idx] = { tmp_handle, mem_offset, nullptr };

                if (peer_idx < (comm_size - 2)) {
                    // proxy data to right peer
                    sendmsg_call(right_peer_socket, recv_fd, mem_offset);
                }
            }
            start_peer_idx++;
        }
        start_peer_idx = 0;
        start_buf_idx++;
    }

    LOG_DEBUG("handles size: ", handles.size(), ", in_buffers size: ", in_buffers.size());

    sched->get_ccl_sched_memory().handle_manager.set(handles);

    status = ccl_sched_entry_status_complete;

    LOG_DEBUG("completed: ", name());
}

int ze_handle_exchange_entry::create_server_socket(const std::string& socket_name,
                                                   struct sockaddr_un* socket_addr,
                                                   int* addr_len,
                                                   int comm_size) {
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
                  socket_name);
    }

    socket_addr->sun_family = AF_UNIX;
    strncpy(socket_addr->sun_path, socket_name.c_str(), sizeof(socket_addr->sun_path) - 1);
    socket_addr->sun_path[sizeof(socket_addr->sun_path) - 1] = '\0';
    *addr_len = sizeof((*socket_addr));

    ret = fcntl(sock, F_SETFL, O_NONBLOCK);
    if (ret) {
        CCL_THROW(
            "fcntl error: ", ret, ", errno: ", strerror(errno), ", socket_name: ", socket_name);
    }

    unlink(socket_name.c_str());

    ret = bind(sock, ((struct sockaddr*)&(*socket_addr)), *addr_len);
    if (ret) {
        CCL_THROW(
            "bind error: ", ret, ", errno: ", strerror(errno), ", socket_name: ", socket_name);
    }

    ret = listen(sock, comm_size);
    if (ret) {
        CCL_THROW(
            "listen error: ", ret, ", errno: ", strerror(errno), ", socket_name: ", socket_name);
    }

    return sock;
}

int ze_handle_exchange_entry::create_client_socket(const std::string& socket_name,
                                                   struct sockaddr_un* socket_addr,
                                                   int* addr_len) {
    memset(&(*socket_addr), 0, sizeof(*(socket_addr)));

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        CCL_THROW("cannot create a client socket: ", sock, ", errno: ", strerror(errno));
    }

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

        if (errno == EMFILE) {
            LOG_TRACE("accept no free fd: ", strerror(errno), ", socket_name: ", socket_name);
            return errno;
        }

        CCL_THROW(
            "accept error: ", strerror(errno), " sock: ", sock, ", socket_name: ", socket_name);
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

int ze_handle_exchange_entry::sendmsg_fd(int sock, int fd, size_t mem_offset) {
    struct msghdr msg = {};
    struct cmsghdr* cmsg;
    char ctrl_buf[CMSG_SPACE(sizeof(fd))] = { 0 };
    struct iovec iov;

    iov.iov_base = &mem_offset;
    iov.iov_len = sizeof(size_t);

    msg.msg_control = ctrl_buf;
    msg.msg_controllen = CMSG_SPACE(sizeof(fd));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    *reinterpret_cast<int*>(CMSG_DATA(cmsg)) = fd;

    ssize_t ret = sendmsg(sock, &msg, 0);
    if (ret < 0) {
        CCL_THROW("sendmsg error: ", ret, ", errno: ", strerror(errno));
        return ret;
    }
    return 0;
}

int ze_handle_exchange_entry::recvmsg_fd(int sock, int& fd, size_t& mem_offset) {
    struct msghdr msg = {};
    struct cmsghdr* cmsg;
    char ctrl_buf[CMSG_SPACE(sizeof(int))] = { 0 };
    struct iovec iov = {};

    size_t buf = {};

    iov.iov_base = &buf;
    iov.iov_len = sizeof(size_t);

    msg.msg_control = ctrl_buf;
    msg.msg_controllen = CMSG_SPACE(sizeof(int));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ssize_t ret = recvmsg(sock, &msg, 0);
    if (ret < 0) {
        LOG_ERROR("recvmsg error: ", ret, ", errno: ", strerror(errno));
        return ret;
    }

    if (msg.msg_flags & (MSG_TRUNC | MSG_CTRUNC)) {
        CCL_THROW("control message is truncated");
    }

    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_len == CMSG_LEN(sizeof(int)) && cmsg->cmsg_level == SOL_SOCKET &&
            cmsg->cmsg_type == SCM_RIGHTS) {
            memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
            break;
        }
    }

    // we assume that the message has a strict format and size, if not this means that something
    // is wrong.
    if (msg.msg_iovlen != 1 || msg.msg_iov[0].iov_len != sizeof(size_t)) {
        CCL_THROW("received data in unexpected format");
    }

    memcpy(&mem_offset, msg.msg_iov[0].iov_base, sizeof(size_t));

    return 0;
}

void ze_handle_exchange_entry::sendmsg_call(int socket_fd, int fd, size_t mem_offset) {
    ssize_t ret = sendmsg_fd(socket_fd, fd, mem_offset);
    if (ret < 0) {
        CCL_THROW("sendmsg_fd error: socket: ",
                  socket_fd,
                  ", fd: ",
                  fd,
                  ", from: ",
                  comm->rank(),
                  ", errno: ",
                  strerror(errno));
    }
    LOG_DEBUG("send: rank[", comm->rank(), "], send fd:", fd, ", socket_fd:", socket_fd);
}

void ze_handle_exchange_entry::recvmsg_call(int socket_fd, int& fd, size_t& mem_offset) {
    int ret = recvmsg_fd(socket_fd, fd, mem_offset);
    if (ret < 0) {
        CCL_THROW("recvmsg_fd error: socket: ",
                  socket_fd,
                  ", from: ",
                  comm->rank(),
                  ", ret: ",
                  ret,
                  ", errno: ",
                  strerror(errno));
    }
    LOG_DEBUG("recv: rank[", rank, "], gotten fd:", fd, ", socket_fd:", socket_fd);
}

int ze_handle_exchange_entry::get_fd_from_handle(const ze_ipc_mem_handle_t* handle, int* fd) {
    memcpy(fd, handle, sizeof(*fd));
    return 0;
}

int ze_handle_exchange_entry::get_handle_from_fd(int* fd, ze_ipc_mem_handle_t* handle) {
    memcpy(handle, static_cast<void*>(fd), sizeof(*fd));
    return 0;
}

void ze_handle_exchange_entry::get_handle(ze_context_handle_t context,
                                          const void* buffer,
                                          ze_ipc_mem_handle_t* handle) {
    CCL_THROW_IF_NOT(context != nullptr);
    CCL_THROW_IF_NOT(buffer != nullptr);

    ZE_CALL(zeMemGetIpcHandle(context, buffer, handle));
}

static size_t get_ptr_diff(const void* ptr1, const void* ptr2) {
    return static_cast<const char*>(ptr2) - static_cast<const char*>(ptr1);
}

std::pair<void*, size_t> ze_handle_exchange_entry::get_mem_info(ze_context_handle_t context,
                                                                void* ptr) {
    void* base_ptr = nullptr;
    size_t alloc_size = 0;

    ZE_CALL(zeMemGetAddressRange(context, ptr, &base_ptr, &alloc_size));

    LOG_DEBUG("zeMemGetAddressRange: ptr: ",
              ptr,
              ", base ptr: ",
              base_ptr,
              ", offset: ",
              get_ptr_diff(base_ptr, ptr),
              ", alloc size: ",
              alloc_size);

    return { base_ptr, static_cast<char*>(ptr) - static_cast<char*>(base_ptr) };
}

void ze_handle_exchange_entry::unlink_sockets() {
    unlink(left_peer_socket_name.c_str());
}

void ze_handle_exchange_entry::close_sockets() {
    close(left_peer_connect_socket);
    close(left_peer_socket);
    close(right_peer_socket);
}

#endif // CCL_ENABLE_SYCL && MULTI_GPU_SUPPORT
