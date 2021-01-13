#include "sched/queue/queue.hpp"

size_t ccl_sched_queue::get_idx() const {
    return idx;
}

void ccl_sched_bin::add(ccl_sched* sched) {}

size_t ccl_sched_bin::erase(size_t idx, size_t& next_idx) {
    return 0;
}

ccl_sched_queue::ccl_sched_queue(size_t idx, std::vector<size_t> atl_eps) : idx(), atl_eps() {}

ccl_sched_queue::~ccl_sched_queue() {}

void ccl_sched_queue::add(ccl_sched* sched) {}

size_t ccl_sched_queue::erase(ccl_sched_bin* bin, size_t idx) {
    return 0;
}

ccl_sched_bin* ccl_sched_queue::peek() {
    return nullptr;
}

std::vector<ccl_sched_bin*> ccl_sched_queue::peek_all() {
    return std::vector<ccl_sched_bin*>{};
}

void ccl_sched_queue::clear() {}
