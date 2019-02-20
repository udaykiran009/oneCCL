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

struct mlsl_sched_queue;
struct mlsl_sched_queue_bin;
struct mlsl_request;
struct mlsl_parallelizer;
class mlsl_sched;
class mlsl_executor;

namespace out_of_order
{
class ooo_match;
}

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

struct mlsl_sched_coll_attr
{
    mlsl_prologue_fn_t prologue_fn;
    mlsl_epilogue_fn_t epilogue_fn;
    mlsl_reduction_fn_t reduction_fn;
    size_t priority;
    int synchronous;
    char match_id[MLSL_MATCH_ID_MAX_LEN];
    int to_cache;
};

struct mlsl_sched_coll_param
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
    mlsl_sched() = default;
    explicit mlsl_sched(mlsl_sched_coll_param& params) : coll_param(params)
    {}

    ~mlsl_sched();

    mlsl_sched& operator = (const mlsl_sched& other);

    //sched preparation and start methods
    void set_coll_attr(const mlsl_coll_attr_t *attr);

    void commit(mlsl_parallelizer* parallelizer);

    mlsl_request* start(mlsl_executor* exec);

    void prepare_partial_scheds(bool dump_scheds = false);

    void update_id()
    {
        sched_id = coll_param.comm->get_sched_id();
    }

    /**
     * Reset runtime parameters and all entries
     */
    void reset();

    /**
     * Reset completion counter of @b req and assign @b req to each partial sched
     * @return pointer to req that can be used to track completion
     */
    mlsl_request* reset_request();

    //collective creation methods
    void add_entry(std::shared_ptr<sched_entry> entry)
    {
        entry->set_exec_mode(exec_mode);
        entries.push_back(entry);
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

    void dump(const char *name) const;

    bool first_progress = true;
    mlsl_sched_queue_bin *bin = nullptr;
    mlsl_sched_coll_param coll_param{};
    mlsl_sched_coll_attr coll_attr{};

    mlsl_sched_memory memory;
    mlsl_sched_entry_exec_mode exec_mode = mlsl_sched_entry_exec_regular;

    size_t idx = 0;  /* index to start */
    std::deque<std::shared_ptr<sched_entry>> entries{};

    mlsl_sched_id_t sched_id = 0;   /* sequence number of the schedule in the communicator */
    mlsl_request *req = nullptr;

    std::vector<std::shared_ptr<mlsl_sched>> partial_scheds{};

    mlsl_sched* root = nullptr;

private:
    void swap(mlsl_sched& other);
};

mlsl_status_t mlsl_sched_progress(mlsl_sched_queue_bin* bin,
                                  size_t max_sched_count,
                                  size_t& completed_sched_count);

mlsl_status_t mlsl_sched_alloc_buffer(mlsl_sched *sched, size_t size, void** ptr);
mlsl_status_t mlsl_sched_free_buffers(mlsl_sched *sched);


const char *mlsl_reduction_to_str(mlsl_reduction_t type);
size_t mlsl_sched_get_priority(mlsl_sched *sched);
mlsl_status_t mlsl_sched_start_subsched(mlsl_sched* sched, mlsl_sched* subsched, mlsl_request **req);
