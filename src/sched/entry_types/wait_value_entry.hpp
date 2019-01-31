#pragma once

#include "sched/entry_types/entry.hpp"

class wait_value_entry : public sched_entry
{
public:
    wait_value_entry() = delete;
    wait_value_entry(mlsl_sched* sched,
                     const volatile uint64_t* ptr,
                     uint64_t expected_value,
                     mlsl_condition condition) :
        sched_entry(sched, true), ptr(ptr),
        expected_value(expected_value), condition(condition)
    {}

    void start_derived()
    {
        MLSL_LOG(DEBUG, "WAIT_VALUE entry current_val %lu, expected_val %lu",
                 *ptr, expected_value);
        status = mlsl_sched_entry_status_started;
        update_derived();
    }

    void update_derived()
    {
        if (condition == mlsl_condition_greater_or_equal && *ptr >= expected_value)
        {
            status = mlsl_sched_entry_status_complete;
        }
        else if (condition == mlsl_condition_equal && *ptr == expected_value)
        {
            status = mlsl_sched_entry_status_complete;
        }
        else
        {
            MLSL_LOG(TRACE, "waiting WAIT_VALUE");
            //_mm_pause();
        }
    }

    const char* name() const
    {
        return "WAIT_VALUE";
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf, "ptr %p, expected_value %lu, condition %d\n",
                                     ptr, expected_value, condition);
        return dump_buf + bytes_written;
    }

private:
    const volatile uint64_t* ptr;
    uint64_t expected_value;
    mlsl_condition condition;
};
