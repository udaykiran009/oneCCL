#pragma once

#include "sched/entry_types/entry.hpp"

class function_entry : public sched_entry
{
public:
    function_entry() = delete;
    function_entry(mlsl_sched* sched,
                   mlsl_sched_entry_function_t fn,
                   const void* ctx) :
        sched_entry(sched), fn(fn), ctx(ctx)
    {}

    void start_derived()
    {
        fn(ctx);
        status = mlsl_sched_entry_status_complete;
    }

    const char* name() const
    {
        return "FUNCTION";
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf, "fn %p, ctx %p\n", fn, ctx);
        return dump_buf + bytes_written;
    }

private:
    mlsl_sched_entry_function_t fn;
    const void* ctx;
};
