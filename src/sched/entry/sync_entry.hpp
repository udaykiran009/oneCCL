#pragma once

#include "common/global/global.hpp"
#include "common/utils/sync_object.hpp"
#include "common/utils/yield.hpp"
#include "sched/entry/entry.hpp"

#include <memory>

class sync_entry : public sched_entry
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "SYNC";
    }

    sync_entry() = delete;
    explicit sync_entry(ccl_sched* sched,
                        std::shared_ptr<sync_object> sync) :
        sched_entry(sched, true), sync(sync)
    {
    }

    void start() override
    {
        status = ccl_sched_entry_status_started;
    }

    void update() override
    {
        if ((sched->get_start_idx() == start_idx) && should_visit)
        {
            /* ensure intra-schedule barrier before inter-schedule barrier */
            sync->visit();
            should_visit = false;
        }

        auto counter = sync->value();
        if (counter == 0)
        {
            status = ccl_sched_entry_status_complete;
        }
        else
        {
            LOG_TRACE("waiting SYNC entry cnt ", counter);
            ccl_yield(ccl::global_data::env().yield_type);
        }
    }

    void reset(size_t idx) override
    {
        sched_entry::reset(idx);
        sync->reset();
        should_visit = true;
    }

    const char* name() const override
    {
        return class_name();
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
    bool should_visit = true;
};
