#include "sched/extra_sched.hpp"
#include "sched/queue/flow_control.hpp"
#include "sched/sched.hpp"

ccl::flow_control::flow_control() {}

ccl::flow_control::~flow_control() {}

ccl_sched::ccl_sched(const ccl_coll_param& coll_param, ccl_request* master_request)
        : ccl_sched_base() {}

ccl_sched::~ccl_sched() {}

void ccl_sched::do_progress() {}

bool ccl_sched::is_strict_order_satisfied() {
    return true;
}

void ccl_sched::complete() {}

void ccl_sched::renew(bool need_update_id /* = false*/) {}

void ccl_sched::add_barrier() {}

ccl_request* ccl_sched::start_subsched(ccl_extra_sched* subsched) {
    return nullptr;
}

void ccl_sched::dump(std::ostream& out) const {}

size_t ccl_sched::entries_count() const {
    return 0;
}

ccl_comm_id_t ccl_sched::get_comm_id() {
    return {};
}
