#pragma once

#include "sched/sched_base.hpp"
#include "sched/sched_timer.hpp"
#include "sched/queue/flow_control.hpp"
#include "internal_types.hpp"

//todo: sequence diagram
//workflow:
//1. new ccl_sched()
//2. set_coll_attr [opt]
//3. sched->commit(parallelizer)
//4. sched->start(executor)
//  4.1 prepare_partial_scheds()
//      4.1.1 update_id()
//      4.1.2 renew()
//  4.2 reset_request()

enum ccl_sched_in_bin_status {
    ccl_sched_in_bin_none,
    ccl_sched_in_bin_added,
    ccl_sched_in_bin_erased
};

typedef ccl::status (*ccl_sched_finalize_fn_t)(ccl_sched*, const void*);

class ccl_extra_sched;
class ccl_master_sched;

class alignas(CACHELINE_SIZE) ccl_sched : public ccl_sched_base {
public:
    static constexpr const char* class_name() {
        return "worker_sched";
    }

    ccl_sched(const ccl_sched_create_param& param,
              ccl_request* master_request,
              ccl_master_sched* master_sched = nullptr);
    ccl_sched() = delete;
    ccl_sched(const ccl_sched& other) = delete;
    ccl_sched& operator=(const ccl_sched& other) = delete;

    virtual ~ccl_sched();

    bool is_strict_order_satisfied();

    void do_progress();

    /**
     * Called after all the entries have been completed
     */
    virtual void complete();

    size_t get_start_idx() const {
        return start_idx;
    }

    /* communicators on build and execution stages can differ */
    ccl_comm_id_t get_comm_id();

    void set_op_id(ccl_op_id_t id) {
        op_id = id;
    }

    ccl_op_id_t get_op_id() {
        return op_id;
    }

    void set_in_bin_status(ccl_sched_in_bin_status status) {
        in_bin_status = status;
    }

    ccl_sched_in_bin_status get_in_bin_status() const {
        return in_bin_status;
    }

    /**
     * Reset runtime parameters and all entries
     */
    void renew(bool need_update_id = false);

    using ccl_sched_base::add_entry_front_t;
    using ccl_sched_base::add_entry_back_t;
    using add_entry_default_t = add_entry_mode_t<ccl_sched_add_mode_last_value>;

    sched_entry* add_entry(std::unique_ptr<sched_entry>&& entry) {
        entry->set_exec_mode(exec_mode);

        sched_entry* raw_ptr = entry.get();
        if (add_mode == ccl_sched_add_back)
            entries.push_back(std::move(entry));
        else if (add_mode == ccl_sched_add_front)
            entries.push_front(std::move(entry));
        else
            CCL_FATAL("unexpected mode ", add_mode);

        return raw_ptr;
    }

    /**
     * Policy-based add_entry
     */
    sched_entry* add_entry(std::unique_ptr<sched_entry>&& entry,
                           add_entry_mode_t<ccl_sched_add_mode_last_value>) {
        return add_entry(std::move(entry));
    }

    sched_entry* add_entry(std::unique_ptr<sched_entry>&& entry, add_entry_front_t) {
        entry->set_exec_mode(exec_mode);

        sched_entry* raw_ptr = entry.get();
        entries.push_front(std::move(entry));
        return raw_ptr;
    }

    sched_entry* add_entry(std::unique_ptr<sched_entry>&& entry, add_entry_back_t) {
        entry->set_exec_mode(exec_mode);

        sched_entry* raw_ptr = entry.get();
        entries.push_back(std::move(entry));
        return raw_ptr;
    }

    /**
     * Require that all previously added entries are completed before subsequent ops
     * may begin execution
     */
    void add_barrier();

    std::vector<ccl::event>& get_deps() const;

    ccl_sched_bin* bin = nullptr; /* valid only during execution */
    ccl_sched_queue* queue = nullptr; /* cached pointer to queue, valid even after execution */
    size_t start_idx = 0; /* index to start */

    /*
      used for unique ATL tag creation in algorithms with multiple parallel sub-schedules
      set once and then used for all entries
    */
    ccl_op_id_t op_id = 0;

    /* to track status of schedule wrt execution bin, not atomic as updated by single thread in time */
    ccl_sched_in_bin_status in_bin_status = ccl_sched_in_bin_none;

    using sched_entry_ptr = std::unique_ptr<sched_entry>;
    std::deque<sched_entry_ptr> entries{};

    /* whether sched should be executed in the same order as in user code */
    /* currently applicable for start phase only */
    bool strict_order = false;

    /*
      limits number of active entries
      mostly makes sense for ATL entries
    */
    ccl::flow_control flow_control;

    void set_finalize_fn(ccl_sched_finalize_fn_t fn, void* ctx) {
        finalize_fn = fn;
        finalize_fn_ctx = ctx;
    }
    ccl_request* req = nullptr;
    // pointer to a master schedule this sched belongs to. If this sched is a partial sched, then
    // it's the same as the req ptr above. But it's going to be different if the sched is created
    // as part of extra_sched.
    // Currently we only set this ptr to non-null when we need it, i.e. these are the cases when we
    // construct sched and entries to run collective. There are some other cases where we don't need
    // master_sched, so it's not set there.
    ccl_master_sched* master_sched = nullptr;
    void dump(std::ostream& out) const;
    size_t entries_count() const;

private:
    ccl_sched_finalize_fn_t finalize_fn = nullptr;
    void* finalize_fn_ctx = nullptr;

    ccl::sched_timer timer;
};
