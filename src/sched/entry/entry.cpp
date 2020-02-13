#include "sched/entry/entry.hpp"
#include "common/log/log.hpp"

void sched_entry::do_progress()
{
    if (status < ccl_sched_entry_status_started)
    {
        CCL_ASSERT(status == ccl_sched_entry_status_not_started ||
                   status == ccl_sched_entry_status_again,
                   "bad status ", status);
        start();
        CCL_ASSERT(status >= ccl_sched_entry_status_again,
                   "bad status ", status);
    }
    else if (status == ccl_sched_entry_status_started)
    {
        LOG_TRACE("update entry ", name());
        update();
        CCL_ASSERT(status >= ccl_sched_entry_status_started,
                   "bad status ", status);
    }

    if (status == ccl_sched_entry_status_complete &&
        exec_mode == ccl_sched_entry_exec_once)
    {
        status = ccl_sched_entry_status_complete_once;
    }
}

bool sched_entry::is_completed()
{
    return (status == ccl_sched_entry_status_complete ||
            status == ccl_sched_entry_status_complete_once);
}

void sched_entry::update()
{
    /*
       update is required for communication/synchronization/wait_value ops
       for other ops it is empty method
    */
}

void sched_entry::reset(size_t idx)
{
    if (status == ccl_sched_entry_status_complete_once)
    {
        return;
    }

    start_idx = idx;
    status = ccl_sched_entry_status_not_started;
}

bool sched_entry::is_strict_order_satisfied()
{
    return (status > ccl_sched_entry_status_not_started);
}

void sched_entry::dump(std::stringstream& str,
                       size_t idx) const
{
    // update with the longest name
    const int entry_name_w = 14;

    ccl_logger::format(str,
                       "[", std::left, std::setw(3), idx, "] ", std::left, std::setw(entry_name_w), name(),
                       " entry, status ", status_to_str(status),
                       " is_barrier ", std::left, std::setw(5), barrier ? "TRUE" : "FALSE", " ");
    dump_detail(str);
}

void sched_entry::make_barrier()
{
    barrier = true;
}

bool sched_entry::is_barrier() const
{
    return barrier;
}

ccl_sched_entry_status sched_entry::get_status() const
{
    return status;
}

void sched_entry::set_status(ccl_sched_entry_status s)
{
    status = s;
}

void sched_entry::set_exec_mode(ccl_sched_entry_exec_mode mode)
{
    exec_mode = mode;
}

void sched_entry::dump_detail(std::stringstream& str) const
{}

void sched_entry::update_status(atl_status_t atl_status)
{
    if (unlikely(atl_status != ATL_STATUS_SUCCESS))
    {
        if (atl_status == ATL_STATUS_AGAIN)
        {
            status = ccl_sched_entry_status_again;
            return;
        }

        CCL_THROW("%s entry failed. atl_status: ", name(), atl_status_to_str(atl_status));
    }
    else
    {
        status = ccl_sched_entry_status_started;
    }
}

const char* sched_entry::status_to_str(ccl_sched_entry_status status)
{
    switch (status)
    {
        case ccl_sched_entry_status_not_started:
            return "NOT_STARTED";
        case ccl_sched_entry_status_again:
            return "AGAIN";
        case ccl_sched_entry_status_started:
            return "STARTED";
        case ccl_sched_entry_status_complete:
            return "COMPLETE";
        case ccl_sched_entry_status_complete_once:
            return "COMPLETE_ONCE";
        case ccl_sched_entry_status_failed:
            return "FAILED";
        case ccl_sched_entry_status_invalid:
            return "INVALID";
        default:
            return "UNKNOWN";
    }
}
