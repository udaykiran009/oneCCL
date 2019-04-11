#pragma once

#include "common/datatype/datatype.hpp"
#include "sched/entry_types/postponed_fields.hpp"
#include "common/utils/utils.hpp"

#include <memory>

typedef mlsl_status_t(* mlsl_sched_entry_function_t)(const void*);

struct mlsl_sched;

enum mlsl_sched_entry_exec_mode
{
    mlsl_sched_entry_exec_regular = 0,
    mlsl_sched_entry_exec_once = 1
};

//todo: enum class
enum mlsl_sched_entry_status
{
    mlsl_sched_entry_status_not_started = 0,
    mlsl_sched_entry_status_started = 1,
    mlsl_sched_entry_status_complete = 2,
    mlsl_sched_entry_status_complete_once = 3, // should has higher value than 'complete'
    mlsl_sched_entry_status_failed = 4,
    mlsl_sched_entry_status_invalid = 5
};

enum mlsl_condition
{
    mlsl_condition_equal = 0,
    mlsl_condition_not_equal = 1,
    mlsl_condition_less = 2,
    mlsl_condition_greater = 3,
    mlsl_condition_less_or_equal = 4,
    mlsl_condition_greater_or_equal = 5
};

class alignas(CACHELINE_SIZE) sched_entry
{
public:
    sched_entry() = delete;
    explicit sched_entry(mlsl_sched* sched,
                         bool is_barrier = false) :
        sched(sched),
        barrier(is_barrier),
        pfields(this)
    {}

    virtual ~sched_entry() {}

    void set_field_fn(mlsl_sched_entry_field_id id,
                      mlsl_sched_entry_field_function_t fn,
                      const void* ctx,
                      bool update_once = true);
    void start();
    virtual void start_derived() = 0;

    void update();
    virtual void update_derived();

    virtual void reset();

    void dump(std::stringstream& str,
              size_t idx) const;
    virtual void* get_field_ptr(mlsl_sched_entry_field_id id);

    void make_barrier();
    bool is_barrier() const;
    mlsl_sched_entry_status get_status() const;
    void set_status(mlsl_sched_entry_status s);
    void set_exec_mode(mlsl_sched_entry_exec_mode mode);

    virtual const char* name() const = 0;

protected:

    virtual void dump_detail(std::stringstream& str) const;
    void check_exec_mode();
    const char* entry_status_to_str(mlsl_sched_entry_status status) const;

    mlsl_sched* sched = nullptr;
    bool barrier = false;
    postponed_fields pfields;
    mlsl_sched_entry_status status = mlsl_sched_entry_status_not_started;
    mlsl_sched_entry_exec_mode exec_mode = mlsl_sched_entry_exec_regular;
};
