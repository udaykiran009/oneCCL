#pragma once

#include <stddef.h>
#include <string>
#include <utility>

namespace ccl {
namespace utils {

struct fd_info {
    size_t open_fd_count;
    size_t max_fd_count;
};

std::string to_string(const fd_info& info);

fd_info get_fd_info();

} // namespace utils
} // namespace ccl
