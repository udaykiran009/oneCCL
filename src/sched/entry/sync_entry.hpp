#pragma once

#include "sched/entry/entry.hpp"
#include "sched/sync_object.hpp"

#include <memory>

class sync_entry : public sched_entry
{
public:
    static constexpr const char *entry_class_name() noexcept
    {
        return "SYNC";
    }

    sync_entry() = default;
    explicit sync_entry(ccl_sched* sched,
                        std::shared_ptr<sync_object> sync) :
        sched_entry(sched, true), sync(sync)
    {
    }

    void start_derived() override
    {
        sync->visit();
        status = ccl_sched_entry_status_started;
    }

    void update_derived() override
    {
        auto counter = sync->value();
        if (counter == 0)
        {
            status = ccl_sched_entry_status_complete;
        }
        else
        {
            LOG_TRACE("waiting SYNC entry cnt ", counter);
            ccl_yield(env_data.yield_type);
        }
    }

    void reset() override
    {
        sched_entry::reset();
        sync->reset();
    }

    const char* name() const override
    {
        return entry_class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                            "counter ", sync->value(),
                            "\n");
    }

private:
    std::shared_ptr<sync_object> sync;
};
