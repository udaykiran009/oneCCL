#pragma once

#include "common/datatype/datatype.hpp"
#include "common/utils/utils.hpp"
#include "sched/entry/postponed_fields.hpp"

#include <memory>
#include <chrono>

typedef iccl_status_t(* iccl_sched_entry_function_t)(const void*);

struct iccl_sched;

enum iccl_sched_entry_exec_mode
{
    iccl_sched_entry_exec_regular = 0,
    iccl_sched_entry_exec_once = 1
};

enum iccl_sched_entry_status
{
    iccl_sched_entry_status_not_started = 0,
    iccl_sched_entry_status_started = 1,
    iccl_sched_entry_status_complete = 2,
    iccl_sched_entry_status_complete_once = 3, // should has higher value than 'complete'
    iccl_sched_entry_status_failed = 4,
    iccl_sched_entry_status_invalid = 5
};

enum iccl_condition
{
    iccl_condition_equal = 0,
    iccl_condition_not_equal = 1,
    iccl_condition_less = 2,
    iccl_condition_greater = 3,
    iccl_condition_less_or_equal = 4,
    iccl_condition_greater_or_equal = 5
};

class alignas(CACHELINE_SIZE) sched_entry
{
public:
    sched_entry() = delete;
    explicit sched_entry(iccl_sched* sched,
                         bool is_barrier = false) :
        sched(sched),
        barrier(is_barrier),
        pfields(this)
    {}

    virtual ~sched_entry() {}

    void set_field_fn(iccl_sched_entry_field_id id,
                      iccl_sched_entry_field_function_t fn,
                      const void* ctx,
                      bool update_once = true);
    void start();
    virtual void start_derived() = 0;

    void update();
    virtual void update_derived();

    virtual void reset();

    void dump(std::stringstream& str,
              size_t idx) const;
    virtual void* get_field_ptr(iccl_sched_entry_field_id id);

    void make_barrier();
    bool is_barrier() const;
    iccl_sched_entry_status get_status() const;
    void set_status(iccl_sched_entry_status s);
    void set_exec_mode(iccl_sched_entry_exec_mode mode);

    virtual const char* name() const = 0;

protected:

    virtual void dump_detail(std::stringstream& str) const;
    void check_exec_mode();
    const char* entry_status_to_str(iccl_sched_entry_status status) const;

    iccl_sched* sched = nullptr;
    bool barrier = false;
    postponed_fields pfields;
    iccl_sched_entry_status status = iccl_sched_entry_status_not_started;
    iccl_sched_entry_exec_mode exec_mode = iccl_sched_entry_exec_regular;

#ifdef ENABLE_DEBUG
    using timer_type = std::chrono::system_clock;
    timer_type::duration exec_time{};
    timer_type::time_point start_time{};
    timer_type::time_point complete_time{};
#endif
};
