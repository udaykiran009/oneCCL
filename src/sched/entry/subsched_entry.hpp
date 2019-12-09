#pragma once

#include "sched/entry/entry.hpp"
#include "sched/extra_sched.hpp"
#include "sched/queue/queue.hpp"

class subsched_entry : public sched_entry
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "SUBSCHED";
    }

    subsched_entry() = delete;
    subsched_entry(ccl_sched* sched,
                   ccl_op_id_t op_id,
                   std::function<void(ccl_sched*)> fill_fn,
                   const char* subsched_name = nullptr) :
        sched_entry(sched),
        fill_fn(fill_fn),
        op_id(op_id),
        subsched_name(subsched_name)
    {
        if (subsched_name)
        {
            LOG_DEBUG("subsched name: ", subsched_name);
        }

        subsched.reset(new ccl_extra_sched(sched->coll_param,
                                           sched->sched_id));
        subsched->coll_param.ctype = ccl_coll_internal;
        subsched->set_op_id(op_id);
        fill_fn(subsched.get());
    }

    ~subsched_entry()
    {
        subsched.reset();
    }

    void start() override
    {
        subsched->renew();
        subsched->bin = sched->bin;
        subsched->queue = sched->queue;
        subsched->sched_id = sched->sched_id;
        status = ccl_sched_entry_status_started;
    }

    void update() override
    {
        subsched->do_progress();
        if (subsched->start_idx == subsched->entries.size())
        {
            status = ccl_sched_entry_status_complete;
        }
    }

    const char* name() const override
    {
        return !subsched_name.empty() ? subsched_name.c_str() : class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str, "content:\n");

        for (size_t idx = 0; idx < subsched->entries.size(); ++idx)
        {
            ccl_logger::format(str, "\t");
            subsched->entries[idx]->dump(str, idx);
        }
    }

private:
    std::unique_ptr<ccl_extra_sched> subsched;
    std::function<void(ccl_sched*)> fill_fn;
    ccl_op_id_t op_id;
    std::string subsched_name;
};
