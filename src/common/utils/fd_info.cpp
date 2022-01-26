
#include <dirent.h>
#include <sstream>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/utils/fd_info.hpp"

namespace ccl {
namespace utils {

std::string to_string(const fd_info& info) {
    std::stringstream ss;

    ss << " fd_info: { open: ";
    ss << info.open_fd_count;
    ss << ", max: ";
    ss << info.max_fd_count;
    ss << " }";

    return ss.str();
}

fd_info get_fd_info() {
    fd_info info;
    memset(&info, 0, sizeof(info));

    rlimit lim;
    memset(&lim, 0, sizeof(lim));

    if (getrlimit(RLIMIT_NOFILE, &lim) != 0) {
        return { 0, 0 };
    }

    // soft limit is the max fd count for our process
    info.max_fd_count = lim.rlim_cur;

    // in case we reached the limit on the open fds, we need
    // to close one for opendir to work. At this point we don't
    // care if we close a valid fd, because the limit is already
    // reached and we cannot work further.
    if (close(info.max_fd_count - 1) == 0) {
        // if close returned 0, we assume that we closed a valid
        // fd, so add this number to the count
        info.open_fd_count += 1;
    }

    DIR* d = opendir("/proc/self/fd");
    if (!d)
        return { 0, 0 };

    dirent* e = nullptr;
    while ((e = readdir(d)) != nullptr) {
        info.open_fd_count += 1;
    }

    info.open_fd_count -= 3; // count '.', '..' entries and fd from opendir

    closedir(d);

    return info;
}

} // namespace utils
} // namespace ccl
