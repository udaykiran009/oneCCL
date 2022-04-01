#pragma once

#include "oneapi/ccl/config.h"

#include <map>
#include <string>
#include <vector>

namespace ccl {
namespace ze {

enum class ipc_exchange_mode : int { sockets, drmfd, pidfd };
static std::map<ipc_exchange_mode, std::string> ipc_exchange_names = {
    std::make_pair(ipc_exchange_mode::sockets, "sockets"),
#ifdef CCL_ENABLE_DRM
    std::make_pair(ipc_exchange_mode::drmfd, "drmfd")
#endif // CCL_ENABLE_DRM
    //TODO: std::make_pair(ipc_exchange_mode::pidfd, "pidfd")
};

class fd_manager {
public:
    fd_manager();
    fd_manager(const fd_manager&) = delete;
    fd_manager(fd_manager&&) = delete;
    fd_manager& operator=(const fd_manager&) = delete;
    fd_manager& operator=(fd_manager&&) = delete;
    ~fd_manager();

    static int mem_handle_to_fd(int dev_fd, int fd);
    static int fd_to_mem_handle(int dev_fd, int handle);

    std::vector<int> get_device_fds();

private:
    void exchange_device_fds();
    std::vector<int> setup_device_fds(int local_count, int proc_idx);

    void close_sockets(int local_count, int proc_idx);

    std::string get_shm_filename();
    void* create_shared_memory();
    void barrier(void* mem);

    const int counter_offset = sizeof(int);
    const int size_per_proc = sizeof(pid_t);

    std::vector<int> init_device_fds();

    std::vector<int> all_socks;
    std::vector<pid_t> all_pids;

    std::vector<int> device_fds;
};

} // namespace ze
} // namespace ccl
