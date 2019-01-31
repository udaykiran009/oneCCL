#pragma once

#include "sched/entry_types/entry.hpp"

class nop_entry : public sched_entry
{
public:
    nop_entry(mlsl_sched* sched) : sched_entry(sched)
    {}

    void start_derived()
    {}

    const char* name() const
    {
        return "NOOP";
    }
};
