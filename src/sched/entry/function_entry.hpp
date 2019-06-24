#pragma once

#include "sched/entry/entry.hpp"

class function_entry : public sched_entry
{
public:
    function_entry() = delete;
    function_entry(mlsl_sched* sched,
                   mlsl_sched_entry_function_t fn,
                   const void* ctx) :
        sched_entry(sched), fn(fn), ctx(ctx)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

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
    void dump_detail(std::stringstream& str) const
    {
        mlsl_logger::format(str,
                            "fn ", fn,
                            ", ctx ", ctx,
                            "\n");
    }

private:
    mlsl_sched_entry_function_t fn;
    const void* ctx;
};
