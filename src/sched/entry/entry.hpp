#pragma once

#include "atl/atl.h"
#include "common/datatype/datatype.hpp"
#include "common/utils/utils.hpp"
#include "sched/entry/postponed_fields.hpp"
#include "internal_types.hpp"
#include <chrono>
#include <memory>

typedef ccl::status (*ccl_sched_entry_function_t)(const void*);

class ccl_sched;

enum ccl_sched_entry_exec_mode { ccl_sched_entry_exec_regular, ccl_sched_entry_exec_once };

enum ccl_sched_entry_status {
    ccl_sched_entry_status_not_started,
    ccl_sched_entry_status_again,
    ccl_sched_entry_status_started,
    ccl_sched_entry_status_complete,
    ccl_sched_entry_status_complete_once, // should has higher value than 'complete'
    ccl_sched_entry_status_failed,
    ccl_sched_entry_status_invalid
};

enum ccl_condition {
    ccl_condition_equal,
    ccl_condition_not_equal,
    ccl_condition_less,
    ccl_condition_greater,
    ccl_condition_less_or_equal,
    ccl_condition_greater_or_equal
};

class alignas(CACHELINE_SIZE) sched_entry {
public:
    sched_entry() = delete;
    explicit sched_entry(ccl_sched* sched, bool is_barrier = false)
            : sched(sched),
              barrier(is_barrier) {}

    virtual ~sched_entry() {}

    void do_progress();
    bool is_completed();

    virtual void reset(size_t start_idx);

    virtual bool is_strict_order_satisfied();

    void dump(std::stringstream& str, size_t idx) const;

    void make_barrier();
    bool is_barrier() const;
    ccl_sched_entry_status get_status() const;
    void set_status(ccl_sched_entry_status s);
    void set_exec_mode(ccl_sched_entry_exec_mode mode);

    virtual const char* name() const = 0;

    static const char* status_to_str(ccl_sched_entry_status status);

protected:
    virtual void start() = 0;
    virtual void update();

    virtual void dump_detail(std::stringstream& str) const;
    const char* entry_status_to_str(ccl_sched_entry_status status) const;
    void update_status(atl_status_t atl_status);

    ccl_sched* sched = nullptr;
    bool barrier = false;
    size_t start_idx = 0;
    ccl_sched_entry_status status = ccl_sched_entry_status_not_started;
    ccl_sched_entry_exec_mode exec_mode = ccl_sched_entry_exec_regular;
};
