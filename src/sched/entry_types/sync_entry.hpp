#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sync_object.hpp"

#include <memory>

class sync_entry : public sched_entry
{
public:
    sync_entry() = delete;
    explicit sync_entry(std::shared_ptr<sync_object> sync_obj) : sync(sync_obj)
    {
        barrier = true;
    }

    void execute()
    {
        if (status == mlsl_sched_entry_status_not_started)
        {
            MLSL_LOG(DEBUG, "starting SYNC entry");
            sync->visit();
            status = mlsl_sched_entry_status_started;
        }

        if (status == mlsl_sched_entry_status_started)
        {
            auto counter = sync->value();
            if (counter == 0)
            {
                MLSL_LOG(DEBUG, "completed SYNC entry");
                status = mlsl_sched_entry_status_complete;
            }
            else
            {
                MLSL_LOG(TRACE, "waiting SYNC entry cnt %zu", counter);
                _mm_pause();
            }
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

    std::shared_ptr<sched_entry> clone() const
    {
        //need to find out how to create new sync_object because full copy has no sense
        throw std::runtime_error("sync_entry clone is not implemented");
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
