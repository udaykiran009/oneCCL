#pragma once

#include "sched/entry/entry.hpp"
#include "atl/atl.h"
#include "coll/coll.hpp"

#include <memory>

#include <deque>
#include <list>

typedef ccl_status_t(*ccl_sched_finalize_fn_t) (ccl_sched*, const void*);

class ccl_sched_queue;
class ccl_sched_bin;
class ccl_request;
class ccl_parallelizer;
class ccl_executor;

enum ccl_sched_internal_type
{
    ccl_sched_internal_none = 0,
    ccl_sched_internal_fusion,
    ccl_sched_internal_ooo
};

enum ccl_sched_add_mode
{
    ccl_sched_add_front = 0,
    ccl_sched_add_back
};

struct ccl_sched_buffer_handler
{
    ccl_buffer buffer;
    size_t size;

    ccl_sched_buffer_handler(ccl_buffer buffer, size_t size)
        : buffer(buffer), size(size) {} 
};

struct ccl_sched_memory
{
    std::list<ccl_sched_buffer_handler> buf_list;
    std::list<atl_mr_t*> mr_list;
};

struct ccl_coll_attr
{
    ccl_prologue_fn_t prologue_fn;
    ccl_epilogue_fn_t epilogue_fn;
    ccl_reduction_fn_t reduction_fn;
    size_t priority;
    int synchronous;
    int to_cache;
    std::string match_id;
};

struct ccl_coll_sparse_param
{
    const void* snd_val_buf;
    size_t snd_val_count;
    void** rcv_ind_buf;
    void** rcv_val_buf;
    size_t* rcv_val_count;
    ccl_datatype_internal_t itype;
};

struct ccl_coll_param
{
    ccl_coll_type ctype;
    void *buf;
    const void* send_buf;
    void* recv_buf;
    size_t count;
    size_t send_count;
    size_t *recv_counts;
    ccl_datatype_internal_t dtype;
    ccl_reduction_t reduction;
    size_t root;
    ccl_comm *comm;
    ccl_coll_sparse_param sparse_param;
};

//todo: sequence diagram
//workflow:
//1. new ccl_sched()
//2. set_coll_attr [opt]
//3. sched->commit(parallelizer)
//4. sched->start(executor)
//  4.1 prepare_partial_scheds()
//      4.1.1 update_id()
//      4.1.2 reset()
//  4.2 reset_request()

class alignas(CACHELINE_SIZE) ccl_sched
{
public:

    ccl_sched(ccl_coll_param& coll_param)
        : coll_param(coll_param)
    {
        alloc_req();
    }

    ccl_sched() = delete;
    ccl_sched(const ccl_sched& other) = delete;
    ccl_sched& operator= (const ccl_sched& other) = delete;

    ~ccl_sched();

    void do_progress();

    void set_coll_attr(const ccl_coll_attr_t* attr,
                       std::string match_id);

    void update_coll_param(ccl_coll_param& param);
    void update_coll_attr(const ccl_coll_attr_t* attr);

    void commit(ccl_parallelizer* parallelizer);

    ccl_request* start(ccl_executor* exec,
                        bool reset_sched = true);

    void complete();

    void add_partial_sched(ccl_coll_param& param);

    void set_request(ccl_request* req);

    void prepare_partial_scheds();

    /**
     * Reset runtime parameters and all entries
     */
    void reset();

    /**
     * Reset completion counter of @b req
     * @return pointer to req that can be used to track completion
     */
    ccl_request* reset_request();

    void add_entry(std::shared_ptr<sched_entry> entry)
    {
        entry->set_exec_mode(exec_mode);

        if (add_mode == ccl_sched_add_back)
            entries.push_back(entry);
        else if (add_mode == ccl_sched_add_front)
            entries.push_front(entry);
        else
            CCL_FATAL("unexpected mode ", add_mode);
    }

    /**
     * Require that all previously added entries are completed before subsequent ops
     * may begin execution
     */
    void add_barrier();

    /**
     * Synchronizes partial schedules on local barrier
     */
    void sync_partial_scheds();

    void set_entry_exec_mode(ccl_sched_entry_exec_mode mode)
    {
        exec_mode = mode;
    }

    void set_add_mode(ccl_sched_add_mode mode)
    {
        add_mode = mode;
    }

    void set_finalize_fn(ccl_sched_finalize_fn_t fn, void* ctx)
    {
        finalize_fn = fn;
        finalize_fn_ctx = ctx;
    }

    ccl_buffer alloc_buffer(size_t size);
    void free_buffers();
    size_t get_priority();
    ccl_request* start_subsched(ccl_sched* subsched);

    void dump_all() const;

    bool first_progress = true;
    ccl_sched_bin* bin = nullptr; /* valid only during execution */
    ccl_sched_queue* queue = nullptr; /* cached pointer to queue, valid even after execution */
    ccl_coll_param coll_param{};
    ccl_coll_attr coll_attr{};

    size_t start_idx = 0;  /* index to start */
    std::deque<std::shared_ptr<sched_entry>> entries{};

    ccl_sched_id_t sched_id = 0;   /* sequence number of the schedule in the communicator */
    ccl_request* req = nullptr;

    std::vector<std::shared_ptr<ccl_sched>> partial_scheds{};

    ccl_sched_memory memory;

    /* whether sched was created by internal module (fusion_manager/ooo_manager) */
    ccl_sched_internal_type internal_type = ccl_sched_internal_none;

    /* whether sched was once checked for completion from user level (by wait/test) */
    bool urgent = false;

    /* whether sched should be started in the same order as in user code */
    bool strict_start_order = false;

private:

    ccl_sched_entry_exec_mode exec_mode = ccl_sched_entry_exec_regular;
    ccl_sched_add_mode add_mode = ccl_sched_add_back;

    ccl_sched_finalize_fn_t finalize_fn = nullptr;
    void* finalize_fn_ctx;

    /* whether req is owned by this schedule or was set externally */
    bool is_own_req = true;

#ifdef ENABLE_DEBUG
    using timer_type = std::chrono::system_clock;
    timer_type::time_point exec_start_time{};
    timer_type::time_point exec_complete_time{};
#endif

    void update_id()
    {
        sched_id = coll_param.comm->get_sched_id(internal_type != ccl_sched_internal_none);
    }

    void dump(const char *name) const;
    void alloc_req();
};

ccl_status_t ccl_bin_progress(ccl_sched_bin* bin,
                                size_t max_sched_count,
                                size_t& completed_sched_count);

void ccl_sched_progress(ccl_sched* sched);
