#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched_queue.hpp"
#include <memory>

class chain_call_entry : public sched_entry
{
public:
    chain_call_entry() = delete;
    chain_call_entry(mlsl_sched* sched,
                     std::function<void(mlsl_sched*)> sched_fill_function,
                     const char* name = nullptr) :
        sched_entry(sched), entry_name(name)
    {
        LOG_DEBUG("creating ", this->name(), " entry");
        //do not mix std::shared_ptr<T>.reset() and mlsl_sched->reset()
        work_sched.reset(new mlsl_sched(sched->coll_param));
        work_sched->coll_param.ctype = mlsl_coll_internal;
        sched_fill_function(work_sched.get());
    }

    void start_derived()
    {
        if (status == mlsl_sched_entry_status_not_started)
        {
            work_sched->reset();
            work_sched->bin = sched->bin;
            work_sched->queue = sched->queue;
            mlsl_sched_progress(work_sched.get());

            if (work_sched->start_idx == work_sched->entries.size())
            {
                status = mlsl_sched_entry_status_complete;
            }
            else
            {
                status = mlsl_sched_entry_status_started;
            }
        }
    }

    void update_derived()
    {
        mlsl_sched_progress(work_sched.get());

        if (work_sched->start_idx == work_sched->entries.size())
        {
            status = mlsl_sched_entry_status_complete;
        }
    }

    const char* name() const
    {
        return entry_name ? entry_name : "CHAIN";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        mlsl_logger::format(str, "content:\n");

        for (size_t idx = 0; idx < work_sched->entries.size(); ++idx)
        {
            mlsl_logger::format(str, "\t");
            work_sched->entries[idx]->dump(str, idx);
        }
    }

private:
    std::unique_ptr<mlsl_sched> work_sched;
    const char* entry_name;
};
