#pragma once

#include "sched/entry/entry.hpp"

class wait_value_entry : public sched_entry
{
public:
    wait_value_entry() = delete;
    wait_value_entry(ccl_sched* sched,
                     const volatile uint64_t* ptr,
                     uint64_t expected_value,
                     ccl_condition condition) :
        sched_entry(sched, true), ptr(ptr),
        expected_value(expected_value), condition(condition)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
    {
        LOG_DEBUG("WAIT_VALUE entry current_val ", *ptr, ", expected_val ", expected_value);
        status = ccl_sched_entry_status_started;
        update_derived();
    }

    void update_derived()
    {
        if (condition == ccl_condition_greater_or_equal && *ptr >= expected_value)
        {
            status = ccl_sched_entry_status_complete;
        }
        else if (condition == ccl_condition_equal && *ptr == expected_value)
        {
            status = ccl_sched_entry_status_complete;
        }
        else
        {
            LOG_TRACE("waiting WAIT_VALUE");
            //_mm_pause();
        }
    }

    const char* name() const
    {
        return "WAIT_VALUE";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        ccl_logger::format(str,
                            "ptr ", ptr,
                            ", expected_value ", expected_value,
                            ", condition ", condition,
                            "\n");
    }

private:
    const volatile uint64_t* ptr;
    uint64_t expected_value;
    ccl_condition condition;
};
