#pragma once
#include <string>

namespace ccl {
struct context_comm_addr {
    size_t thread_idx = 0;
    size_t thread_count = 0;
    size_t comm_rank = 0;
    size_t comm_size = 0;

    std::string to_string() const;
};
}
