#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "common/utils/exchange_utils.hpp"
#include "common/utils/fd_info.hpp"

namespace ccl {
namespace utils {

bool allgather(std::shared_ptr<atl_base_comm> comm,
               const void* send_buf,
               void* recv_buf,
               int bytes,
               bool sync) {
    std::vector<int> recv_bytes(comm->get_size(), bytes);
    return allgatherv(comm, send_buf, recv_buf, recv_bytes, sync);
}

bool allgatherv(std::shared_ptr<atl_base_comm> comm,
                const void* send_buf,
                void* recv_buf,
                const std::vector<int>& recv_bytes,
                bool sync) {
    atl_req_t req{};
    bool ret = true;
    int comm_rank = comm->get_rank();
    int comm_size = comm->get_size();

    CCL_THROW_IF_NOT((int)recv_bytes.size() == comm->get_size(),
                     "unexpected recv_bytes size ",
                     recv_bytes.size(),
                     ", comm_size ",
                     comm_size);

    std::vector<int> offsets(comm_size, 0);
    for (int i = 1; i < comm_size; i++) {
        offsets[i] = offsets[i - 1] + recv_bytes[i - 1];
    }

    comm->allgatherv(0 /* ep_idx */,
                     send_buf,
                     recv_bytes[comm_rank],
                     recv_buf,
                     recv_bytes.data(),
                     offsets.data(),
                     req);
    if (sync) {
        comm->wait(0 /* ep_idx */, req);
    }
    else {
        CCL_THROW("unexpected sync parameter");
    }
    return ret;
}

int check_msg_retval(std::string operation_name,
                     ssize_t bytes,
                     struct iovec iov,
                     struct msghdr msg,
                     size_t union_size,
                     int sock,
                     int fd) {
    LOG_DEBUG(operation_name,
              ": ",
              bytes,
              ", expected_bytes:",
              iov.iov_len,
              ", expected size of cntr_buf: ",
              union_size,
              " -> gotten cntr_buf: ",
              msg.msg_controllen,
              ", socket: ",
              sock,
              ", fd: ",
              fd);
    int ret = -1;
    if (bytes == static_cast<ssize_t>(iov.iov_len)) {
        ret = 0;
    }
    else if (bytes < 0) {
        ret = -errno;
    }
    else {
        ret = -EIO;
    }
    return ret;
}

void sendmsg_fd(int sock, int fd, void* payload, int payload_len) {
    CCL_THROW_IF_NOT(fd >= 0, "unexpected fd value");
    char empty_buf;
    struct iovec iov;
    memset(&iov, 0, sizeof(iov));
    if (!payload) {
        iov.iov_base = &empty_buf;
        iov.iov_len = 1;
    }
    else {
        iov.iov_base = payload;
        iov.iov_len = payload_len;
    }

    union {
        struct cmsghdr align;
        char cntr_buf[CMSG_SPACE(sizeof(int))]{};
    } u;

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_control = u.cntr_buf;
    msg.msg_controllen = sizeof(u.cntr_buf);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    auto cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    *(int*)CMSG_DATA(cmsg) = fd;

    ssize_t send_bytes = sendmsg(sock, &msg, 0);
    CCL_THROW_IF_NOT(
        !check_msg_retval("sendmsg", send_bytes, iov, msg, sizeof(u.cntr_buf), sock, fd),
        " errno: ",
        strerror(errno));
}

void recvmsg_fd(int sock, int* fd, void* payload, int payload_len) {
    CCL_THROW_IF_NOT(fd != nullptr, "unexpected fd value");
    char empty_buf;
    struct iovec iov;
    memset(&iov, 0, sizeof(iov));
    if (!payload) {
        iov.iov_base = &empty_buf;
        iov.iov_len = 1;
    }
    else {
        iov.iov_base = payload;
        iov.iov_len = payload_len;
    }

    union {
        struct cmsghdr align;
        char cntr_buf[CMSG_SPACE(sizeof(int))]{};
    } u;

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_control = u.cntr_buf;
    msg.msg_controllen = sizeof(u.cntr_buf);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ssize_t recv_bytes = recvmsg(sock, &msg, 0);
    CCL_THROW_IF_NOT(
        !check_msg_retval("recvmsg", recv_bytes, iov, msg, sizeof(u.cntr_buf), sock, *fd),
        " errno: ",
        strerror(errno));

    if (msg.msg_flags & (MSG_CTRUNC | MSG_TRUNC)) {
        std::string flag_str = "";
        if (msg.msg_flags & MSG_CTRUNC) {
            flag_str += " MSG_CTRUNC";
        }
        if (msg.msg_flags & MSG_TRUNC) {
            flag_str += " MSG_TRUNC";
        }

        /** MSG_CTRUNC message can be in case of:
         * - remote peer send invalid fd, so msg_controllen == 0
         * - limit of fds reached in the current process, so msg_controllen == 0
         * - the remote peer control message > msg_control buffer size
         */
        CCL_THROW("control or usual message is truncated:",
                  flag_str,
                  " control message size: ",
                  msg.msg_controllen,
                  ", ",
                  to_string(ccl::utils::get_fd_info()));
    }

    for (auto cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_len == CMSG_LEN(sizeof(int)) && cmsg->cmsg_level == SOL_SOCKET &&
            cmsg->cmsg_type == SCM_RIGHTS) {
            memcpy(fd, CMSG_DATA(cmsg), sizeof(int));
            break;
        }
    }

    // we assume that the message has a strict format and size, if not this means that something
    // is wrong.
    size_t expected_len = 1;
    if (payload) {
        expected_len = payload_len;
    }
    if (msg.msg_iov[0].iov_len != expected_len) {
        CCL_THROW("received data in unexpected format");
    }
}

void sendmsg_call(int sock, int fd, void* payload, int payload_len, const int rank) {
    sendmsg_fd(sock, fd, payload, payload_len);
    LOG_DEBUG("send: rank[", rank, "], send fd: ", fd, ", sock: ", sock);
}

void recvmsg_call(int sock, int* fd, void* payload, int payload_len, const int rank) {
    recvmsg_fd(sock, fd, payload, payload_len);
    LOG_DEBUG("recv: rank[", rank, "], got fd: ", fd, ", sock: ", sock);
}
} // namespace utils
} // namespace ccl
