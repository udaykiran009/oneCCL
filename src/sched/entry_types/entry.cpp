#include "sched/entry_types/entry.hpp"

void sched_entry::set_field_fn(mlsl_sched_entry_field_id id,
                               mlsl_sched_entry_field_function_t fn,
                               const void* ctx,
                               bool update_once)
{
    pfields.add(id, fn, ctx, update_once);
}

void sched_entry::start()
{
    MLSL_LOG(DEBUG, "starting %s entry", name());
    MLSL_ASSERTP(status == mlsl_sched_entry_status_not_started);
    pfields.update();
    start_derived();
    MLSL_ASSERTP(status >= mlsl_sched_entry_status_started);
    if (status == mlsl_sched_entry_status_complete)
        MLSL_LOG(DEBUG, "completed %s entry", name());
    check_exec_mode();
}

void sched_entry::update()
{
    if (status != mlsl_sched_entry_status_started) return;
    MLSL_ASSERTP(status == mlsl_sched_entry_status_started);
    update_derived();
    MLSL_ASSERTP(status >= mlsl_sched_entry_status_started);
    if (status == mlsl_sched_entry_status_complete)
        MLSL_LOG(DEBUG, "completed %s entry", name());
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

char* sched_entry::dump(char* dump_buf, size_t idx) const
{
    auto bytes_written = sprintf(dump_buf, "[%3zu]  %-7s entry, status %s, is_barrier %-5s, ",
                                 idx, name(), entry_status_to_str(status), barrier ? "TRUE" : "FALSE");
    return dump_detail(dump_buf + bytes_written);
}

void* sched_entry::get_field_ptr(mlsl_sched_entry_field_id id)
{
    MLSL_ASSERTP(0);
    return nullptr;
}

void sched_entry::make_barrier() { barrier = true; }
bool sched_entry::is_barrier() const { return barrier; }
mlsl_sched_entry_status sched_entry::get_status() const { return status; }
void sched_entry::set_status(mlsl_sched_entry_status s) { status = s; }
void sched_entry::set_exec_mode(mlsl_sched_entry_exec_mode mode) { exec_mode = mode; }
char* sched_entry::dump_detail(char* dump_buf) const { return dump_buf; }

void sched_entry::check_exec_mode()
{
    if (status == mlsl_sched_entry_status_complete &&
        exec_mode == mlsl_sched_entry_exec_once)
        status = mlsl_sched_entry_status_complete_once;
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
