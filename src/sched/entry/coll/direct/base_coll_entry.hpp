#pragma once

#include "sched/entry/entry.hpp"

class base_coll_entry : public sched_entry {
public:
    base_coll_entry() = delete;
    base_coll_entry(ccl_sched* sched) : sched_entry(sched) {
        sched->strict_order = true;
    }

    bool is_strict_order_satisfied() override {
        return (status == ccl_sched_entry_status_started || is_completed());
    }
};
