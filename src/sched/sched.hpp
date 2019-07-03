#pragma once

#include "sched/entry/entry.hpp"
#include "atl/atl.h"
#include "coll/coll.hpp"

#include <memory>

#include <deque>
#include <list>

typedef iccl_status_t(*iccl_sched_finalize_fn_t) (iccl_sched*, const void*);

class iccl_sched_queue;
class iccl_sched_bin;
class iccl_request;
class iccl_parallelizer;
class iccl_executor;

enum iccl_sched_internal_type
{
    iccl_sched_internal_none = 0,
    iccl_sched_internal_fusion,
    iccl_sched_internal_ooo
};

enum iccl_sched_add_mode
{
    iccl_sched_add_front = 0,
    iccl_sched_add_back
};

struct iccl_sched_buffer_handler
{
    void *ptr;
    size_t size;

    iccl_sched_buffer_handler(void* ptr, size_t size)
        : ptr(ptr), size(size) {} 
};

struct iccl_sched_memory
{
    std::list<iccl_sched_buffer_handler> buf_list;
    std::list<atl_mr_t*> mr_list;
};

struct iccl_coll_attr
{
    iccl_prologue_fn_t prologue_fn;
    iccl_epilogue_fn_t epilogue_fn;
    iccl_reduction_fn_t reduction_fn;
    size_t priority;
    int synchronous;
    int to_cache;
    std::string match_id;
};

struct iccl_coll_sparse_param
{
    const void *snd_val_buf;
    size_t snd_val_count;
    void **rcv_ind_buf;
    void **rcv_val_buf;
    size_t *rcv_val_count;
    iccl_datatype_internal_t itype;
};

struct iccl_coll_param
{
    iccl_coll_type ctype;
    void *buf;
    const void *send_buf;
    void *recv_buf;
    size_t count;
    size_t send_count;
    size_t *recv_counts;
    iccl_datatype_internal_t dtype;
    iccl_reduction_t reduction;
    size_t root;
    iccl_comm *comm;
    iccl_coll_sparse_param sparse_param;
};

//todo: sequence diagram
//workflow:
//1. new iccl_sched()
//2. set_coll_attr [opt]
//3. sched->commit(parallelizer)
//4. sched->start(executor)
//  4.1 prepare_partial_scheds()
//      4.1.1 update_id()
//      4.1.2 reset()
//  4.2 reset_request()

class alignas(CACHELINE_SIZE) iccl_sched
{
public:

    iccl_sched(iccl_coll_param& coll_param)
        : coll_param(coll_param)
    {
        alloc_req();
    }

    iccl_sched() = delete;
    iccl_sched(const iccl_sched& other) = delete;
    iccl_sched& operator= (const iccl_sched& other) = delete;

    ~iccl_sched();

    void do_progress();

    void set_coll_attr(const iccl_coll_attr_t* attr,
                        std::string match_id);

    void commit(iccl_parallelizer* parallelizer);

    iccl_request* start(iccl_executor* exec,
                            bool reset_sched = true);

    void complete();

    void add_partial_sched(iccl_coll_param& coll_param);

    void set_request(iccl_request* req);

    void prepare_partial_scheds();

    /**
     * Reset runtime parameters and all entries
     */
    void reset();

    /**
     * Reset completion counter of @b req
     * @return pointer to req that can be used to track completion
     */
    iccl_request* reset_request();

    void add_entry(std::shared_ptr<sched_entry> entry)
    {
        entry->set_exec_mode(exec_mode);

        if (add_mode == iccl_sched_add_back)
            entries.push_back(entry);
        else if (add_mode == iccl_sched_add_front)
            entries.push_front(entry);
        else
            ICCL_FATAL("unexpected mode ", add_mode);
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

    void set_entry_exec_mode(iccl_sched_entry_exec_mode mode)
    {
        exec_mode = mode;
    }

    void set_add_mode(iccl_sched_add_mode mode)
    {
        add_mode = mode;
    }

    void set_finalize_fn(iccl_sched_finalize_fn_t fn, void* ctx)
    {
        finalize_fn = fn;
        finalize_fn_ctx = ctx;
    }

    void* alloc_buffer(size_t size);
    void free_buffers();
    size_t get_priority();
    iccl_request* start_subsched(iccl_sched* subsched);

    void dump_all() const;

    bool first_progress = true;
    iccl_sched_bin* bin = nullptr; /* valid only during execution */
    iccl_sched_queue* queue = nullptr; /* cached pointer to queue, valid even after execution */
    iccl_coll_param coll_param{};
    iccl_coll_attr coll_attr{};

    iccl_sched_memory memory;

    size_t start_idx = 0;  /* index to start */
    std::deque<std::shared_ptr<sched_entry>> entries{};

    iccl_sched_id_t sched_id = 0;   /* sequence number of the schedule in the communicator */
    iccl_request *req = nullptr;

    std::vector<std::shared_ptr<iccl_sched>> partial_scheds{};

    /* whether sched was created by internal module (fusion_manager/ooo_manager) */
    iccl_sched_internal_type internal_type = iccl_sched_internal_none;

    /* whether sched was once checked for completion from user level (by wait/test) */
    bool urgent = false;

    /* whether sched should be started in the same order as in user code */
    bool strict_start_order = false;

private:

    iccl_sched_entry_exec_mode exec_mode = iccl_sched_entry_exec_regular;
    iccl_sched_add_mode add_mode = iccl_sched_add_back;

    iccl_sched_finalize_fn_t finalize_fn = nullptr;
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
        sched_id = coll_param.comm->get_sched_id(internal_type != iccl_sched_internal_none);
    }

    void dump(const char *name) const;
    void alloc_req();
};

iccl_status_t iccl_bin_progress(iccl_sched_bin* bin,
                                size_t max_sched_count,
                                size_t& completed_sched_count);

void iccl_sched_progress(iccl_sched* sched);
