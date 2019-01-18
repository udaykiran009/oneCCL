#pragma once

#include "common/datatype/datatype.hpp"
#include "common/log/log.hpp"

#include <memory>

struct mlsl_sched;

//todo: enum class
enum mlsl_sched_entry_status
{
    mlsl_sched_entry_status_not_started = 0,
    mlsl_sched_entry_status_started = 1,
    mlsl_sched_entry_status_complete = 2,
    mlsl_sched_entry_status_failed = 3,
    mlsl_sched_entry_status_invalid = 4
};

class sched_entry
{
public:
    sched_entry() = default;
    explicit sched_entry(mlsl_sched* schedule,
                         bool is_barrier = false) : sched(schedule), barrier(is_barrier)
    {}
    virtual ~sched_entry() {};

    virtual void execute() = 0;

    virtual void adjust(size_t partition_idx,
                        size_t partition_count)
    {}

    char* dump(char* dump_buf,
               size_t idx) const
    {
        auto bytes_written = sprintf(dump_buf, "[%3zu]  %-7s entry, status %s, is_barrier %-5s, ",
                                     idx, name(), entry_status_to_str(status), barrier ? "TRUE" : "FALSE");
        return dump_detail(dump_buf + bytes_written);
    }

    virtual const char* name() const = 0;
    virtual std::shared_ptr<sched_entry> clone() const = 0;

    virtual void reset()
    {
        status = mlsl_sched_entry_status_not_started;
    }

    void make_barrier()
    {
        barrier = true;
    }

    bool is_barrier()
    {
        return barrier;
    }

    mlsl_sched_entry_status get_status() const
    {
        return status;
    }

protected:

    virtual char* dump_detail(char* dump_buf) const = 0;

    void get_count_and_offset(size_t count,
                              mlsl_datatype_internal_t dtype,
                              size_t part_idx,
                              size_t part_count,
                              size_t& out_count,
                              size_t& out_offset)
    {
        MLSL_FATAL("Not supported yet");
        out_count = count / part_count;
        out_offset = part_idx * out_count * mlsl_datatype_get_size(dtype);
        if (part_idx == (part_count - 1))
        {
            out_count += count % part_count;
        }
    }

    template<typename T>
    void adjust_ptr(T* ptr,
                    size_t offset)
    {
        ptr = static_cast<char*>(ptr) + offset;
    }

    template<typename T>
    void adjust_ptr(const T* ptr,
                    size_t offset)
    {
        ptr = static_cast<char*>(const_cast<T*>(ptr)) + offset;
    }

    const char* entry_status_to_str(mlsl_sched_entry_status status) const
    {
        switch (status)
        {
            case mlsl_sched_entry_status_not_started:
                return "NOT_STARTED";
            case mlsl_sched_entry_status_started:
                return "STARTED";
            case mlsl_sched_entry_status_complete:
                return "COMPLETE";
            case mlsl_sched_entry_status_failed:
                return "FAILED";
            case mlsl_sched_entry_status_invalid:
                return "INVALID";
            default:
                return "UNKNOWN";
        }
    }

    mlsl_sched* sched = nullptr;
    bool barrier = false;
    mlsl_sched_entry_status status = mlsl_sched_entry_status_not_started;
};
