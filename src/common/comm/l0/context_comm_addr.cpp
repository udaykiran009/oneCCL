#include <sstream>

#include "common/comm/l0/context_comm_addr.hpp"
namespace ccl {

std::string context_comm_addr::to_string() const {
    std::stringstream ss;
    ss << "thread(" << thread_idx << "/" << thread_count << "), rank(" << comm_rank << "/"
       << comm_size << ")";
    return ss.str();
}
}
