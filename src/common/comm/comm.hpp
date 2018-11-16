#pragma once

#include "mlsl_types.hpp"

#include <unordered_map>
#include <stdint.h>

typedef uint16_t mlsl_comm_id_t;
typedef uint16_t mlsl_sched_id_t;
typedef uint64_t mlsl_atl_comm_tag_t;

//key = rank, value = global rank
using rank_to_global_rank_map = std::unordered_map<size_t, size_t>;

struct mlsl_comm
{
    size_t rank;
    size_t size;
    size_t pof2;
    mlsl_comm_id_t comm_id;
    mlsl_comm_id_t next_sched_id;
};

/* use global comm for all operations */
extern mlsl_comm *global_comm;

mlsl_status_t mlsl_comm_create_internal(size_t rank, size_t size, mlsl_comm **comm,
                                        rank_to_global_rank_map&& ranks_map);

mlsl_status_t mlsl_comm_get_global_rank(mlsl_comm* comm, size_t rank, size_t* global_rank);

mlsl_status_t mlsl_comm_id_acquire(mlsl_comm_id_t* out_id);

mlsl_status_t mlsl_comm_id_release(mlsl_comm_id_t id);

mlsl_sched_id_t mlsl_comm_get_sched_id(mlsl_comm *comm);

mlsl_atl_comm_tag_t mlsl_create_atl_tag(mlsl_comm_id_t comm_id, mlsl_sched_id_t sched_id, size_t rank);
