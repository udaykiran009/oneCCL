#pragma once

#include "common/global/global.hpp"
#include "common/utils/yield.hpp"
#include "sched/entry/entry.hpp"

class wait_value_entry : public sched_entry
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "WAIT_VALUE";
    }

    wait_value_entry() = delete;
    wait_value_entry(ccl_sched* sched,
                     const volatile uint64_t* ptr,
                     uint64_t expected_value,
                     ccl_condition condition) :
        sched_entry(sched, true), ptr(ptr),
        expected_value(expected_value), condition(condition)
    {
    }

    void start() override
    {
        LOG_DEBUG("WAIT_VALUE entry current_val ", *ptr, ", expected_val ", expected_value);
        status = ccl_sched_entry_status_started;
        update();
    }

    void update() override
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
            ccl_yield(ccl::global_data::env().yield_type);
        }
    }

    const char* name() const override
    {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
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
