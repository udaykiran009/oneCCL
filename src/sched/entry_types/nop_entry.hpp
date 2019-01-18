#pragma once

#include "sched/entry_types/entry.hpp"

class nop_entry : public sched_entry
{
public:
    nop_entry() = default;

    void execute()
    {
        //nothing to do here
    }

    const char* name() const
    {
        return "";
    }

    std::shared_ptr<sched_entry> clone() const
    {
        //full member-wise copy
        return std::make_shared<nop_entry>(*this);
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        //nothing to do here
        return dump_buf;
    }
};
