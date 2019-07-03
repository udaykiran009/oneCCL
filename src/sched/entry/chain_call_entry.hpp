#pragma once

#include "sched/entry/entry.hpp"
#include "sched/sched_queue.hpp"
#include <memory>

class chain_call_entry : public sched_entry
{
public:
    chain_call_entry() = delete;
    chain_call_entry(iccl_sched* sched,
                     std::function<void(iccl_sched*)> sched_fill_function,
                     const char* name = nullptr) :
        sched_entry(sched), entry_name(name)
    {
        LOG_DEBUG("creating ", this->name(), " entry");
        //do not mix std::shared_ptr<T>.reset() and iccl_sched->reset()
        work_sched.reset(new iccl_sched(sched->coll_param));
        work_sched->coll_param.ctype = iccl_coll_internal;
        sched_fill_function(work_sched.get());
    }

    void start_derived()
    {
        if (status == iccl_sched_entry_status_not_started)
        {
            work_sched->reset();
            work_sched->bin = sched->bin;
            work_sched->queue = sched->queue;
            iccl_sched_progress(work_sched.get());

            if (work_sched->start_idx == work_sched->entries.size())
            {
                status = iccl_sched_entry_status_complete;
            }
            else
            {
                status = iccl_sched_entry_status_started;
            }
        }
    }

    void update_derived()
    {
        iccl_sched_progress(work_sched.get());

        if (work_sched->start_idx == work_sched->entries.size())
        {
            status = iccl_sched_entry_status_complete;
        }
    }

    const char* name() const
    {
        return entry_name ? entry_name : "CHAIN";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        iccl_logger::format(str, "content:\n");

        for (size_t idx = 0; idx < work_sched->entries.size(); ++idx)
        {
            iccl_logger::format(str, "\t");
            work_sched->entries[idx]->dump(str, idx);
        }
    }

private:
    std::unique_ptr<iccl_sched> work_sched;
    const char* entry_name;
};
