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

typedef mlsl_status_t(*mlsl_sched_entry_postponed_field_function_t) (void*, void*);

struct mlsl_sched_queue;
struct mlsl_sched_queue_bin;
struct mlsl_coll_param;
struct mlsl_sched;
struct mlsl_request;

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

class mlsl_sched
{
public:
    mlsl_sched() = default;
    explicit mlsl_sched(mlsl_sched_coll_param& params) : coll_param(params)
    {}

    ~mlsl_sched();

    mlsl_sched& operator = (const mlsl_sched& other);

    bool first_progress = true;
    mlsl_sched_queue_bin *bin = nullptr;
    mlsl_sched_coll_param coll_param{};
    mlsl_sched_coll_attr coll_attr{};

    mlsl_sched_memory memory;
    mlsl_sched_entry_exec_mode exec_mode = mlsl_sched_entry_exec_regular;

    size_t idx = 0;  /* index to start */
    std::deque<std::shared_ptr<sched_entry>> entries;

    mlsl_sched_id_t sched_id = 0;   /* sequence number of the schedule in the communicator */
    mlsl_request *req = nullptr;

    std::vector<std::shared_ptr<mlsl_sched>> partial_scheds{};

    mlsl_sched* root = nullptr;

    mlsl_sched *next = nullptr;   /* linked-list next pointer */
    mlsl_sched *prev = nullptr;   /* linked-list prev pointer */

    void add_entry(std::shared_ptr<sched_entry> entry)
    {
        entry->set_exec_mode(exec_mode);
        entries.push_back(entry);
    }

    void set_coll_attr(const mlsl_coll_attr_t *attr);

private:
    void swap(mlsl_sched& other);
};

mlsl_status_t mlsl_sched_commit(mlsl_sched *sched);
mlsl_status_t mlsl_sched_start(mlsl_sched *sched, mlsl_request **req);
mlsl_status_t mlsl_sched_add_barrier(mlsl_sched *sched);
mlsl_status_t mlsl_sched_sync_schedules(std::vector<std::shared_ptr<mlsl_sched>>& scheds);
mlsl_status_t mlsl_sched_progress(mlsl_sched_queue_bin *bin, size_t sched_count, size_t *processed_sched_count);
mlsl_status_t mlsl_sched_update_id(mlsl_sched *sched);
mlsl_status_t mlsl_sched_dump(mlsl_sched *sched, const char *name);
mlsl_status_t mlsl_sched_reset(mlsl_sched *sched);

mlsl_status_t mlsl_sched_alloc_buffer(mlsl_sched *sched, size_t size, void** ptr);
mlsl_status_t mlsl_sched_free_buffers(mlsl_sched *sched);

mlsl_status_t mlsl_sched_set_entry_exec_mode(mlsl_sched* sched, mlsl_sched_entry_exec_mode mode);

const char *mlsl_reduction_to_str(mlsl_reduction_t type);
size_t mlsl_sched_get_priority(mlsl_sched *sched);
void mlsl_sched_reset_request(mlsl_sched *s, mlsl_request **req);
void mlsl_sched_prepare_partial_scheds(mlsl_sched *sched, bool dump);
mlsl_status_t mlsl_sched_start_subsched(mlsl_sched* sched, mlsl_sched* subsched, mlsl_request **req);
