#pragma once

#include "sched/entry/entry.hpp"

class base_coll_entry : public sched_entry
{
public:
    base_coll_entry() = delete;
    base_coll_entry(mlsl_sched* sched) :
        sched_entry(sched)
    {
        sched->strict_start_order = true;
    }
};
