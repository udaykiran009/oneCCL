#pragma once

#include "atl/atl.h"
#include "common/datatype/datatype.hpp"
#include "common/utils/utils.hpp"
#include "sched/entry/postponed_fields.hpp"
#include <chrono>
#include <memory>

typedef ccl_status_t(* ccl_sched_entry_function_t)(const void*);

class ccl_sched;

enum ccl_sched_entry_exec_mode
{
    ccl_sched_entry_exec_regular = 0,
    ccl_sched_entry_exec_once = 1
};

enum ccl_sched_entry_status
{
    ccl_sched_entry_status_not_started,
    ccl_sched_entry_status_again,
    ccl_sched_entry_status_started,
    ccl_sched_entry_status_complete,
    ccl_sched_entry_status_complete_once, // should has higher value than 'complete'
    ccl_sched_entry_status_failed,
    ccl_sched_entry_status_invalid
};

enum ccl_condition
{
    ccl_condition_equal = 0,
    ccl_condition_not_equal = 1,
    ccl_condition_less = 2,
    ccl_condition_greater = 3,
    ccl_condition_less_or_equal = 4,
    ccl_condition_greater_or_equal = 5
};

class alignas(CACHELINE_SIZE) sched_entry
{
public:
    sched_entry() = delete;
    explicit sched_entry(ccl_sched* sched,
                         bool is_barrier = false) :
        sched(sched),
        barrier(is_barrier)
    {}

    virtual ~sched_entry() {}
    void start();
    virtual void start_derived() = 0;

    void update();
    virtual void update_derived();

    virtual void reset(size_t start_idx);

    virtual bool is_strict_order_satisfied();

    void dump(std::stringstream& str,
              size_t idx) const;

    void make_barrier();
    bool is_barrier() const;
    ccl_sched_entry_status get_status() const;
    void set_status(ccl_sched_entry_status s);
    void set_exec_mode(ccl_sched_entry_exec_mode mode);

    virtual const char* name() const = 0;

    static const char* status_to_str(ccl_sched_entry_status status);

protected:

    virtual void dump_detail(std::stringstream& str) const;
    void check_exec_mode();
    const char* entry_status_to_str(ccl_sched_entry_status status) const;
    void update_status(atl_status_t atl_status);

    ccl_sched* sched = nullptr;
    bool barrier = false;
    size_t start_idx = 0;
    ccl_sched_entry_status status = ccl_sched_entry_status_not_started;
    ccl_sched_entry_exec_mode exec_mode = ccl_sched_entry_exec_regular;

#ifdef ENABLE_TIMERS
    using timer_type = std::chrono::system_clock;
    timer_type::duration exec_time{};
    timer_type::time_point start_time{};
    timer_type::time_point complete_time{};
#endif
};
