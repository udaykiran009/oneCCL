#include "common/log/log.hpp"
#include "sched/queue/flow_control.hpp"

namespace ccl {

flow_control::flow_control()
        : max_credits(CCL_MAX_FLOW_CREDITS),
          min_credits(CCL_MAX_FLOW_CREDITS),
          credits(CCL_MAX_FLOW_CREDITS) {}

flow_control::~flow_control() {
    LOG_DEBUG("max used credits: ", (max_credits - min_credits));
}

void flow_control::set_max_credits(size_t value) {
    max_credits = min_credits = credits = value;
}

size_t flow_control::get_max_credits() const {
    return max_credits;
}

size_t flow_control::get_credits() const {
    return credits;
}

bool flow_control::take_credit() {
    if (credits) {
        credits--;
        CCL_THROW_IF_NOT(
            credits <= max_credits, "unexpected credits ", credits, ", max_credits ", max_credits);
        min_credits = std::min(min_credits, credits);
        return true;
    }
    else {
        LOG_TRACE("no available credits");
        return false;
    }
}

void flow_control::return_credit() {
    credits++;
    CCL_THROW_IF_NOT((credits > 0) && (credits <= max_credits) && (credits > min_credits),
                     "unexpected credits ",
                     credits,
                     ", max_credits ",
                     max_credits,
                     ", min_credits ",
                     min_credits);
}

} // namespace ccl
