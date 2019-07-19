#include "sched/entry/entry.hpp"
#include "common/log/log.hpp"

void sched_entry::set_field_fn(iccl_sched_entry_field_id id,
                               iccl_sched_entry_field_function_t fn,
                               const void* ctx,
                               bool update_once)
{
    pfields.add(id, fn, ctx, update_once);
}

void sched_entry::start()
{
#ifdef ENABLE_DEBUG
    auto start = timer_type::now();
    start_time = start;
#endif

    LOG_DEBUG("starting entry ", name());
    ICCL_ASSERT(status == iccl_sched_entry_status_not_started, "bad status ", status);

    pfields.update();
    start_derived();
    ICCL_ASSERT(status >= iccl_sched_entry_status_started, "bad status ", status);

    if (status == iccl_sched_entry_status_complete)
    {
        LOG_DEBUG("completed entry ", name());
#ifdef ENABLE_DEBUG
        complete_time = timer_type::now();
#endif
    }

    check_exec_mode();

#ifdef ENABLE_DEBUG
    exec_time += timer_type::now() - start;
#endif
}

void sched_entry::update()
{
    if (status != iccl_sched_entry_status_started)
    {
        return;
    }

#ifdef ENABLE_DEBUG
    auto start = timer_type::now();
#endif

    update_derived();

    ICCL_ASSERT(status >= iccl_sched_entry_status_started, "bad status ", status);

    if (status == iccl_sched_entry_status_complete)
    {
        LOG_DEBUG("completed entry ", name());
#ifdef ENABLE_DEBUG
        complete_time = timer_type::now();
#endif
    }

    check_exec_mode();

#ifdef ENABLE_DEBUG
    exec_time += timer_type::now() - start;
#endif
}

void sched_entry::update_derived()
{
    /* update_derived is required for communication/synchronization/wait_value ops
       for other ops it is empty method
    */
}

void sched_entry::reset()
{
    if (status == iccl_sched_entry_status_complete_once)
    {
        return;
    }
    status = iccl_sched_entry_status_not_started;

#ifdef ENABLE_DEBUG
    exec_time = timer_type::duration{};
    start_time = timer_type::time_point{};
    complete_time = start_time;
#endif
}

void sched_entry::dump(std::stringstream& str,
                       size_t idx) const
{
    //update with the longest name
    const int entry_name_w = 12;

#ifdef ENABLE_DEBUG
    auto start = from_time_point(start_time);
    auto end = from_time_point(complete_time);
    auto life_time = std::chrono::duration_cast<std::chrono::microseconds>(complete_time - start_time);

    iccl_logger::format(str,
                        "[", std::left, std::setw(3), idx, "] ", std::left, std::setw(entry_name_w), name(),
                        " entry, status ", entry_status_to_str(status),
                        " is_barrier ", std::left, std::setw(5), barrier ? "TRUE" : "FALSE",
                        " exec_time[us] ", std::setw(5), std::setbase(10),
                        std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count(),
                        " life_time[us] ", std::setw(5), std::setbase(10), status == iccl_sched_entry_status_complete ? life_time.count() : 0,
                        " start[us] 0.", std::setw(5), std::setbase(10), start.tv_nsec / 1000,
                        " end[us] 0.", std::setw(5), std::setbase(10), end.tv_nsec / 1000, " ");
#else
    iccl_logger::format(str,
                       "[", std::left, std::setw(3), idx, "] ", std::left, std::setw(entry_name_w), name(),
                       " entry, status ", entry_status_to_str(status),
                       " is_barrier ", std::left, std::setw(5), barrier ? "TRUE" : "FALSE", " ");

#endif
    dump_detail(str);
}

void* sched_entry::get_field_ptr(iccl_sched_entry_field_id id)
{
    ICCL_FATAL("not supported");
    return nullptr;
}

void sched_entry::make_barrier()
{
    barrier = true;
}
bool sched_entry::is_barrier() const
{
    return barrier;
}
iccl_sched_entry_status sched_entry::get_status() const
{
    return status;
}
void sched_entry::set_status(iccl_sched_entry_status s)
{
    status = s;
}
void sched_entry::set_exec_mode(iccl_sched_entry_exec_mode mode)
{
    exec_mode = mode;
}
void sched_entry::dump_detail(std::stringstream& str) const
{
}

void sched_entry::check_exec_mode()
{
    if (status == iccl_sched_entry_status_complete && exec_mode == iccl_sched_entry_exec_once)
    {
        status = iccl_sched_entry_status_complete_once;
    }
}

const char* sched_entry::entry_status_to_str(iccl_sched_entry_status status) const
{
    switch (status)
    {
        case iccl_sched_entry_status_not_started:
            return "NOT_STARTED";
        case iccl_sched_entry_status_started:
            return "STARTED";
        case iccl_sched_entry_status_complete:
            return "COMPLETE";
        case iccl_sched_entry_status_complete_once:
            return "COMPLETE_ONCE";
        case iccl_sched_entry_status_failed:
            return "FAILED";
        case iccl_sched_entry_status_invalid:
            return "INVALID";
        default:
            return "UNKNOWN";
    }
}
