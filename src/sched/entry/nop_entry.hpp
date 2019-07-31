#pragma once

#include "sched/entry/entry.hpp"

class nop_entry : public sched_entry
{
public:
    static constexpr const char *entry_class_name() noexcept
    {
        return "NOOP";
    }

    nop_entry(ccl_sched* sched) : sched_entry(sched)
    {
    }

    void start_derived() override
    {}

    const char* name() const override
    {
        return entry_class_name();
    }
};
