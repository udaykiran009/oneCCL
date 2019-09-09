#pragma once

#include "sched/entry/entry.hpp"

class function_entry : public sched_entry
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "FUNCTION";
    }

    function_entry() = delete;
    function_entry(ccl_sched* sched,
                   ccl_sched_entry_function_t fn,
                   const void* ctx) :
        sched_entry(sched), fn(fn), ctx(ctx)
    {
    }

    void start() override
    {
        fn(ctx);
        status = ccl_sched_entry_status_complete;
    }

    const char* name() const override
    {
        return "FUNCTION";
    }

protected:
    void dump_detail(std::stringstream& str) const override
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
