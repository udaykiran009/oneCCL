#pragma once

#include "sched/entry_types/entry.hpp"
#include "atl/atl.h"
#include "coll/coll.hpp"
#include "common/comm/comm.hpp"
#include "common/datatype/datatype.hpp"
#include "common/log/log.hpp"
#include "common/request/request.hpp"
#include "common/utils/lock.hpp"

#include <memory>

#include <deque>
#include <list>

typedef mlsl_status_t(*mlsl_sched_finalize_fn_t) (mlsl_sched*, const void*);

struct mlsl_sched_queue;
struct mlsl_sched_queue_bin;
struct mlsl_request;
struct mlsl_parallelizer;
class mlsl_executor;

enum mlsl_sched_add_mode
{
    mlsl_sched_add_front = 0,
    mlsl_sched_add_back
};

struct mlsl_sched_buffer_handler
{
    void *ptr;
    size_t size;

    mlsl_sched_buffer_handler(void* ptr, size_t size)
        : ptr(ptr), size(size) {} 
};

struct mlsl_sched_memory
{
    std::list<mlsl_sched_buffer_handler> buf_list;
    std::list<atl_mr_t*> mr_list;
};

struct mlsl_coll_attr
{
    mlsl_prologue_fn_t prologue_fn;
    mlsl_epilogue_fn_t epilogue_fn;
    mlsl_reduction_fn_t reduction_fn;
    size_t priority;
    int synchronous;
    char match_id[MLSL_MATCH_ID_MAX_LEN];
    int to_cache;
};

struct mlsl_coll_sparse_param
{
    const void *snd_val_buf;
    size_t snd_val_count;
    void **rcv_ind_buf;
    void **rcv_val_buf;
    size_t *rcv_val_count;
    mlsl_datatype_internal_t itype;    
};

struct mlsl_coll_param
{
    mlsl_coll_type ctype;
    void *buf;
    const void *send_buf;
    void *recv_buf;
    size_t count;
    size_t send_count;
    size_t *recv_counts;
    mlsl_datatype_internal_t dtype;
    mlsl_reduction_t reduction;
    size_t root;
    mlsl_comm *comm;
    mlsl_coll_sparse_param sparse_param;
};


//todo: sequence diagram
//workflow:
//1. new mlsl_sched()
//2. set_coll_attr [opt]
//3. sched->commit(parallelizer)
//4. sched->start(executor)
//  4.1 prepare_partial_scheds()
//      4.1.1 update_id()
//      4.1.2 reset()
//      4.1.3 reset_request()

class mlsl_sched
{
public:

    mlsl_sched(mlsl_coll_param& coll_param)
        : coll_param(coll_param)
    {
        alloc_req();
    }

    mlsl_sched() = delete;
    mlsl_sched(const mlsl_sched& other) = delete;
    mlsl_sched& operator= (const mlsl_sched& other) = delete;

    ~mlsl_sched();

    void do_progress();

    void set_coll_attr(const mlsl_coll_attr_t *attr);

    void commit(mlsl_parallelizer* parallelizer);

    mlsl_request* start(mlsl_executor* exec);

    void add_partial_sched(mlsl_coll_param& coll_param);

    void set_request(mlsl_request* req);

    void prepare_partial_scheds(bool dump_scheds = false);

    void update_id()
    {
        sched_id = coll_param.comm->get_sched_id(is_internal);
    }

    /**
     * Reset runtime parameters and all entries
     */
    void reset();

    /**
     * Reset completion counter of @b req
     * @return pointer to req that can be used to track completion
     */
    mlsl_request* reset_request();

    void add_entry(std::shared_ptr<sched_entry> entry)
    {
        entry->set_exec_mode(exec_mode);

        if (add_mode == mlsl_sched_add_back)
            entries.push_back(entry);
        else if (add_mode == mlsl_sched_add_front)
            entries.push_front(entry);
        else
            MLSL_ASSERTP(0);
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

    void set_entry_exec_mode(mlsl_sched_entry_exec_mode mode)
    {
        exec_mode = mode;
    }

    void set_add_mode(mlsl_sched_add_mode mode)
    {
        add_mode = mode;
    }

    void set_finalize_fn(mlsl_sched_finalize_fn_t fn, void* ctx)
    {
        finalize_fn = fn;
        finalize_fn_ctx = ctx;
    }

    void dump(const char *name) const;

    void* alloc_buffer(size_t size);
    void free_buffers();
    size_t get_priority();
    mlsl_request* start_subsched(mlsl_sched* subsched);

    bool first_progress = true;
    mlsl_sched_queue_bin *bin = nullptr;
    mlsl_coll_param coll_param{};
    mlsl_coll_attr coll_attr{};

    mlsl_sched_memory memory;
    mlsl_sched_entry_exec_mode exec_mode = mlsl_sched_entry_exec_regular;
    mlsl_sched_add_mode add_mode = mlsl_sched_add_back;

    size_t start_idx = 0;  /* index to start */
    std::deque<std::shared_ptr<sched_entry>> entries{};

    mlsl_sched_id_t sched_id = 0;   /* sequence number of the schedule in the communicator */
    mlsl_request *req = nullptr;

    std::vector<std::shared_ptr<mlsl_sched>> partial_scheds{};

    /* whether sched was created by internal module (fusion/ooo) */
    bool is_internal = false;

    /* whether sched was once checked for completion from user level (by wait/test) */
    bool urgent = false;

    /* whether req is owned by this schedule or was set externally */
    bool is_own_req = true;

    mlsl_sched_finalize_fn_t finalize_fn = nullptr;
    void* finalize_fn_ctx;

    mlsl_sched* root = nullptr;

private:
    void alloc_req();
};

mlsl_status_t mlsl_sched_progress(mlsl_sched_queue_bin* bin,
                                  size_t max_sched_count,
                                  size_t& completed_sched_count);

const char *mlsl_reduction_to_str(mlsl_reduction_t type);
