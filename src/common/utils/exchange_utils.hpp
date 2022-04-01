#pragma once

#include "atl/atl_base_comm.hpp"

#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <sys/socket.h>
#include <sys/un.h>

namespace ccl {
namespace utils {
bool allgather(std::shared_ptr<atl_base_comm> comm,
               const void* send_buf,
               void* recv_buf,
               int bytes,
               bool sync = true);
bool allgatherv(std::shared_ptr<atl_base_comm> comm,
                const void* send_buf,
                void* recv_buf,
                const std::vector<int>& recv_bytes,
                bool sync = true);

int check_msg_retval(std::string operation_name,
                     ssize_t bytes,
                     struct iovec iov,
                     struct msghdr msg,
                     size_t union_size,
                     int sock,
                     int fd);
void sendmsg_fd(int sock, int fd, void* payload, int payload_len);
void recvmsg_fd(int sock, int* fd, void* payload, int payload_len);

void sendmsg_call(int sock, int fd, void* payload, int payload_len, const int rank);
void recvmsg_call(int sock, int* fd, void* payload, int payload_len, const int rank);

} // namespace utils
} // namespace ccl
