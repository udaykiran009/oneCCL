#pragma once

#include "sched/entry/entry.hpp"
#include "sched/queue/queue.hpp"
#include "sched/extra_sched.hpp"
#include <memory>

class chain_call_entry : public sched_entry
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "CHAIN";
    }

    chain_call_entry() = delete;
    chain_call_entry(ccl_sched* sched,
                     std::function<void(ccl_sched*)> sched_fill_function,
                     const char* name = nullptr) :
        sched_entry(sched), entry_special_name(name)
    {
        if (name)
        {
            LOG_DEBUG("entry object name: ", name);
        }
        work_sched.reset(new ccl_extra_sched(sched->coll_param, sched->sched_id));
        work_sched->coll_param.ctype = ccl_coll_internal;
        sched_fill_function(work_sched.get());
    }

    void start() override
    {
        work_sched->renew();
        work_sched->bin = sched->bin;
        work_sched->queue = sched->queue;
        work_sched->sched_id = sched->sched_id;
        status = ccl_sched_entry_status_started;
    }

    void update() override
    {
        work_sched->do_progress();
        if (work_sched->start_idx == work_sched->entries.size())
        {
            status = ccl_sched_entry_status_complete;
        }
    }

    const char* name() const override
    {
        return !entry_special_name.empty() ? entry_special_name.c_str() : class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str, "content:\n");

        for (size_t idx = 0; idx < work_sched->entries.size(); ++idx)
        {
            ccl_logger::format(str, "\t");
            work_sched->entries[idx]->dump(str, idx);
        }
    }

private:
    std::unique_ptr<ccl_extra_sched> work_sched;
    std::string entry_special_name;
};
