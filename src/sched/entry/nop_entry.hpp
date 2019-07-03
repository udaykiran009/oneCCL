#pragma once

#include "sched/entry/entry.hpp"

class nop_entry : public sched_entry
{
public:
    nop_entry(iccl_sched* sched) : sched_entry(sched)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
    {}

    const char* name() const
    {
        return "NOOP";
    }
};
