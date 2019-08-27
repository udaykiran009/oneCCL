#include "sched/extra_sched.hpp"
#include "sched/master_sched.hpp"
#include "sched/sched_queue.hpp"
#include "sched/sync_object.hpp"
#include "sched/entry_factory.hpp"
#include "common/global/global.hpp"
#include "parallelizer/parallelizer.hpp"

ccl_master_sched::~ccl_master_sched()
{
    for (auto& part_sched: partial_scheds)
    {
        part_sched.reset();
    }

    CCL_ASSERT (memory.mr_list.empty(), "memory list is not empty");
    free_buffers();
}

void ccl_master_sched::commit(ccl_parallelizer* parallelizer)
{
    if (env_data.priority_mode == ccl_priority_lifo)
    {
        coll_attr.priority = ccl_sched_base::get_lifo_priority();
    }

    if (partial_scheds.empty())
    {
        /* single time operations */
        update_id();
        if (parallelizer)
        {
            parallelizer->process(this);
            CCL_ASSERT(!partial_scheds.empty(), "ccl_master_sched must have at least 1 partial sched after parallelized");
        }
    }
    else
    {
        /* repeated operations, should happen each time to reuse schedule */
        for (size_t idx = 0; idx < partial_scheds.size(); idx++)
        {
            partial_scheds[idx]->coll_attr.priority = coll_attr.priority;
        }
    }

    LOG_DEBUG("sched ", this,
              ", sched_id ", sched_id, ", req ", static_cast<const ccl_request *>(this),
              ", partial_sched_count ", partial_scheds.size());
}

ccl_request* ccl_master_sched::start(ccl_executor* exec, bool reset_sched)
{
    /* sanity check the schedule */
    CCL_ASSERT(coll_param.comm);

    LOG_DEBUG("starting schedule ", this, ", type ", ccl_coll_type_to_str(coll_param.ctype));

    prepare_partial_scheds();

    if (reset_sched)
    {
        reset_request();
    }

    if (env_data.sched_dump)
    {
        dump(std::cout);
    }

    exec->start(this);
    return this;
}

ccl_request* ccl_master_sched::reset_request()
{
    set_counter(std::max(1, static_cast<int>(partial_scheds.size())));
    return this;
}

void ccl_master_sched::add_partial_sched(ccl_coll_param& coll_param)
{
    partial_scheds.emplace_back(std::make_shared<ccl_sched>(coll_param, this));
    partial_scheds.back()->internal_type = internal_type;
}

void ccl_master_sched::prepare_partial_scheds()
{
    for (auto& sched: partial_scheds)
    {
        sched->renew(true);
    }
}

void ccl_master_sched::sync_partial_scheds()
{
    CCL_THROW_IF_NOT(!partial_scheds.empty(), "no partial schedules");

    auto sync_obj = std::make_shared<sync_object>(partial_scheds.size());
    for (auto& sched : partial_scheds)
    {
        entry_factory::make_entry<sync_entry>(sched.get(), sync_obj);
    }
}

void ccl_master_sched::dump(std::ostream &out) const
{
    if (!env_data.sched_dump)
    {
        return;
    }

    ccl_sched_base::dump(out, class_name());
    ccl_logger::format(out,
                       ", req: ", static_cast<const ccl_request*>(this),
                       ", worker_sched count: ", partial_scheds.size());

    for (const auto& sched : partial_scheds)
    {
        sched->dump(out);
    }

#ifdef ENABLE_TIMERS
    ccl_logger::format(out, "\nlife time [us] ", std::setw(5), std::setbase(10),
        std::chrono::duration_cast<std::chrono::microseconds>(exec_complete_time - exec_start_time).count(),
        "\n");
#endif
    
    ccl_logger::format(out, "--------------------------------\n");
}
