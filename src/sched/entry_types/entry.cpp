#include "sched/entry_types/entry.hpp"
#include "common/log/log.hpp"

void sched_entry::set_field_fn(mlsl_sched_entry_field_id id,
                               mlsl_sched_entry_field_function_t fn,
                               const void* ctx,
                               bool update_once)
{
    pfields.add(id, fn, ctx, update_once);
}

void sched_entry::start()
{
    LOG_DEBUG("starting entry ", name());
    MLSL_ASSERT(status == mlsl_sched_entry_status_not_started, "bad status ", status);
    pfields.update();
    start_derived();
    MLSL_ASSERT(status >= mlsl_sched_entry_status_started, "bad status ", status);
    if (status == mlsl_sched_entry_status_complete)
        LOG_DEBUG("completed entry ", name());
    check_exec_mode();
}

void sched_entry::update()
{
    if (status != mlsl_sched_entry_status_started) return;
    MLSL_ASSERT(status == mlsl_sched_entry_status_started, "bad status ", status);
    update_derived();
    MLSL_ASSERT(status >= mlsl_sched_entry_status_started, "bad status ", status);
    if (status == mlsl_sched_entry_status_complete)
        LOG_DEBUG("completed entry ", name());
    check_exec_mode();
}

void sched_entry::update_derived()
{
    /* update_derived is required for communication/synchronization/wait_value ops
       for other ops it is empty method
    */
}

void sched_entry::reset()
{
    if (status == mlsl_sched_entry_status_complete_once)
        return;
    status = mlsl_sched_entry_status_not_started;
}

void sched_entry::dump(std::stringstream& str,
                       size_t idx) const
{
    //update with the longest name
    const int entry_name_w = 12;

    mlsl_logger::format(str,
                       "[", std::left, std::setw(3), idx, "] ", std::left, std::setw(entry_name_w), name(),
                       " entry, status ", entry_status_to_str(status),
                       " is_barrier ", std::left, std::setw(5), barrier ? "TRUE" : "FALSE",
                       " ");
    dump_detail(str);
}

void* sched_entry::get_field_ptr(mlsl_sched_entry_field_id id)
{
    MLSL_FATAL("not supported");
    return nullptr;
}

void sched_entry::make_barrier() { barrier = true; }
bool sched_entry::is_barrier() const { return barrier; }
mlsl_sched_entry_status sched_entry::get_status() const { return status; }
void sched_entry::set_status(mlsl_sched_entry_status s) { status = s; }
void sched_entry::set_exec_mode(mlsl_sched_entry_exec_mode mode) { exec_mode = mode; }
void sched_entry::dump_detail(std::stringstream& str) const { }

void sched_entry::check_exec_mode()
{
    if (status == mlsl_sched_entry_status_complete && exec_mode == mlsl_sched_entry_exec_once)
    {
        status = mlsl_sched_entry_status_complete_once;
    }
}

const char* sched_entry::entry_status_to_str(mlsl_sched_entry_status status) const
{
    switch (status)
    {
        case mlsl_sched_entry_status_not_started:
            return "NOT_STARTED";
        case mlsl_sched_entry_status_started:
            return "STARTED";
        case mlsl_sched_entry_status_complete:
            return "COMPLETE";
        case mlsl_sched_entry_status_complete_once:
            return "COMPLETE_ONCE";
        case mlsl_sched_entry_status_failed:
            return "FAILED";
        case mlsl_sched_entry_status_invalid:
            return "INVALID";
        default:
            return "UNKNOWN";
    }
}
