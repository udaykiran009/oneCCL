#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sync_object.hpp"

#include <memory>

class sync_entry : public sched_entry
{
public:
    sync_entry() = delete;
    explicit sync_entry(mlsl_sched* sched,
                        std::shared_ptr<sync_object> sync_obj) :
        sched_entry(sched, true), sync(sync_obj)
    {}

    void start_derived()
    {
        sync->visit();
        status = mlsl_sched_entry_status_started;
    }

    void update_derived()
    {
        auto counter = sync->value();
        if (counter == 0)
        {
            status = mlsl_sched_entry_status_complete;
        }
        else
        {
            MLSL_LOG(TRACE, "waiting SYNC entry cnt %zu", counter);
            _mm_pause();
        }
    }

    void reset() override
    {
        sched_entry::reset();
        sync->reset();
    }

    const char* name() const
    {
        return "SYNC";
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf, "counter %zu\n", sync->value());
        return dump_buf + bytes_written;
    }

private:
    std::shared_ptr<sync_object> sync;
};
