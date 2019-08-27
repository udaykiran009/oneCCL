#pragma once

#include "sched/sched_base.hpp"

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
typedef ccl_status_t(*ccl_sched_finalize_fn_t) (ccl_sched*, const void*);

class ccl_extra_sched;

class alignas(CACHELINE_SIZE) ccl_sched : public ccl_sched_base
{
public:
    static constexpr const char* class_name()
    {
        return "worker_sched";
    }   
    
    ccl_sched(const ccl_coll_param& coll_param, ccl_request *master_request)
        : ccl_sched_base(coll_param)
    {
        req = master_request;
    }

    ccl_sched() = delete;
    ccl_sched(const ccl_sched& other) = delete;
    ccl_sched& operator= (const ccl_sched& other) = delete;

    ~ccl_sched();

    bool is_strict_order_satisfied();

    /* check that all entries were executed */
    bool is_executed();

    void start_entries();

    void do_progress();

    void complete();

    size_t get_start_idx() const
    {
        return start_idx;
    }

    /**
     * Reset runtime parameters and all entries
     */
    void renew(bool need_update_id = false);

    sched_entry* add_entry(std::unique_ptr<sched_entry> &&entry)
    {
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
     * Require that all previously added entries are completed before subsequent ops
     * may begin execution
     */
    void add_barrier();

    ccl_request* start_subsched(ccl_extra_sched* subsched);


    bool first_progress = true;
    ccl_sched_bin* bin = nullptr; /* valid only during execution */
    ccl_sched_queue* queue = nullptr; /* cached pointer to queue, valid even after execution */
    size_t start_idx = 0;  /* index to start */

    using sched_entry_ptr = std::unique_ptr<sched_entry>;
    std::deque<sched_entry_ptr> entries{};

    /* whether sched should be started in the same order as in user code */
    bool strict_start_order = false;

    void set_finalize_fn(ccl_sched_finalize_fn_t fn, void* ctx)
    {
        finalize_fn = fn;
        finalize_fn_ctx = ctx;
    }
    ccl_request* req = nullptr;
    void dump(std::ostream &out) const;
private:
    ccl_sched_finalize_fn_t finalize_fn = nullptr;
    void* finalize_fn_ctx;


#ifdef ENABLE_TIMERS
    using timer_type = std::chrono::system_clock;
    timer_type::time_point exec_start_time{};
    timer_type::time_point exec_complete_time{};
#endif
};

ccl_status_t ccl_bin_progress(ccl_sched_bin* bin,
                              size_t& completed_sched_count);
