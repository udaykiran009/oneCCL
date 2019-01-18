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

#define MLSL_POSTPONED_COUNT  ((size_t)(0xFFFFFFFFFFFFFFFF))
#define MLSL_POSTPONED_ADDR   ((void*)(0xFFFFFFFFFFFFFFFF))

struct mlsl_sched_queue;
struct mlsl_sched_queue_bin;
struct mlsl_coll_param;
struct mlsl_sched;

namespace out_of_order
{
class ooo_match;
}

enum mlsl_sched_memory_type
{
    mlsl_sched_memory_buffer     = 0,
    mlsl_sched_memory_registered = 1
};

struct mlsl_sched_memory
{
    mlsl_sched_memory_type type;
    void *ptr;

    mlsl_sched_memory *next;
    mlsl_sched_memory *prev;
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

struct mlsl_sched_postponed_fields
{
    void* buf;
    size_t count;
    mlsl_datatype_internal dtype;
};

struct mlsl_sched
{
    mlsl_sched() = default;
    ~mlsl_sched();
    mlsl_sched(const mlsl_sched& other);
    mlsl_sched& operator = (const mlsl_sched& other);

    bool first_progress = true;
    mlsl_sched_queue_bin *bin = nullptr;
    mlsl_sched_coll_param coll_param{};
    mlsl_sched_coll_attr coll_attr{};

    size_t idx = 0;  /* index into entries array of first yet-outstanding entry */ /* index to start */
    std::deque<std::shared_ptr<sched_entry>> entries;

    mlsl_sched_id_t sched_id = 0;   /* sequence number of the schedule in the communicator */
    mlsl_request *req = nullptr;
    mlsl_sched_memory *persistent_memory = nullptr;

    mlsl_sched_postponed_fields postponed_fields{};

    mlsl_sched **partial_scheds{};
    size_t partial_sched_count = 0;

    mlsl_sched* root = nullptr;

    mlsl_sched *next = nullptr;   /* linked-list next pointer */
    mlsl_sched *prev = nullptr;   /* linked-list prev pointer */

    void add_entry(std::shared_ptr<sched_entry> entry)
    {
        entries.push_back(entry);
    }

private:
    void swap(mlsl_sched& other);
};

mlsl_status_t mlsl_sched_commit(mlsl_sched *sched);
mlsl_status_t mlsl_sched_start(mlsl_sched *sched, mlsl_request **req);

mlsl_status_t mlsl_sched_add_barrier(mlsl_sched *sched);
mlsl_status_t mlsl_sched_sync_schedules(mlsl_sched **scheds, size_t count);

mlsl_status_t mlsl_sched_bcast(void *buf, size_t count, mlsl_datatype_internal_t dtype,
                               size_t root, mlsl_comm* comm, mlsl_sched **sched);
mlsl_status_t mlsl_sched_reduce(const void *send_buf, void *recv_buf, size_t count, mlsl_datatype_internal_t dtype,
                                mlsl_reduction_t reduction, size_t root, mlsl_comm* comm, mlsl_sched **sched);
mlsl_status_t mlsl_sched_allreduce(const void *send_buf, void *recv_buf, size_t count, mlsl_datatype_internal_t dtype,
                                   mlsl_reduction_t reduction, mlsl_comm* comm, mlsl_sched **sched);
mlsl_status_t mlsl_sched_allgatherv(const void *send_buf, size_t send_count, void *recv_buf, size_t *recv_counts,
                                    mlsl_datatype_internal_t dtype, mlsl_comm* comm, mlsl_sched **sched);
mlsl_status_t mlsl_sched_barrier(mlsl_comm* comm, mlsl_sched **sched);

mlsl_status_t mlsl_sched_tensor_bcast(mlsl_comm* comm, mlsl_sched** sched, bool temporal);

mlsl_status_t mlsl_sched_progress(mlsl_sched_queue_bin *bin, size_t sched_count, size_t *processed_sched_count);
mlsl_status_t mlsl_sched_adjust_entries(mlsl_sched *sched, size_t partition_idx, size_t partition_count);
mlsl_status_t mlsl_sched_adjust_tag(mlsl_sched *sched);
mlsl_status_t mlsl_sched_dump(mlsl_sched *sched, const char *name);
mlsl_status_t mlsl_sched_reset(mlsl_sched *sched);

mlsl_status_t mlsl_sched_add_persistent_memory(mlsl_sched *sched, mlsl_sched_memory_type type, void *ptr);
mlsl_status_t mlsl_sched_free_persistent_memory(mlsl_sched *sched);

const char *mlsl_reduction_to_str(mlsl_reduction_t type);

mlsl_status_t mlsl_sched_set_coll_attr(mlsl_sched *sched, const mlsl_coll_attr_t *attr);

size_t mlsl_sched_get_priority(mlsl_sched *sched);

void mlsl_update_request_reference(mlsl_sched *s, mlsl_request **req);

void mlsl_sched_prepare(mlsl_sched *sched, bool dump);
