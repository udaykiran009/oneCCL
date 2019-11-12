#include "common/request/request.hpp"
#include "sched/entry/coll/coll_entry_helper.hpp"
#include "sched/extra_sched.hpp"

void coll_entry::start()
{
    if (update_fields())
    {
        coll_sched.reset();
    }

    if (!coll_sched)
    {
        ccl_coll_param coll_param{};
        coll_param.comm = sched->coll_param.comm;
        coll_sched.reset(new ccl_extra_sched(coll_param, sched->sched_id));
        auto res = coll_entry_helper::build_schedule(coll_sched.get(), sched, param);
        CCL_ASSERT(res == ccl_status_success, "error during build_schedule, res ", res);
    }

    CCL_THROW_IF_NOT(coll_sched);
    LOG_DEBUG("starting COLL entry");
    auto req = sched->start_subsched(coll_sched.get());
    LOG_DEBUG("COLL entry: sched ", coll_sched.get(), ", req ", req);

    status = ccl_sched_entry_status_started;
}

void coll_entry::update()
{
    CCL_THROW_IF_NOT(coll_sched, "empty request");
    if (coll_sched->is_completed())
    {
        LOG_DEBUG("COLL entry, completed sched: ", coll_sched.get());
        status = ccl_sched_entry_status_complete;
    }
}
