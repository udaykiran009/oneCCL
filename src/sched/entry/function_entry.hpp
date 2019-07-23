#pragma once

#include "sched/entry/entry.hpp"

class function_entry : public sched_entry
{
public:
    function_entry() = delete;
    function_entry(ccl_sched* sched,
                   ccl_sched_entry_function_t fn,
                   const void* ctx) :
        sched_entry(sched), fn(fn), ctx(ctx)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
    {
        fn(ctx);
        status = ccl_sched_entry_status_complete;
    }

    const char* name() const
    {
        return "FUNCTION";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        ccl_logger::format(str,
                           "fn ", (void*)(fn),
                           ", ctx ", ctx,
                           "\n");
    }

private:
    ccl_sched_entry_function_t fn;
    const void* ctx;
};
