#include "common/comm/comm.h"
#include "common/log/log.h"
#include "comm.h"
#include "../log/log.h"
#include "../../../include/mlsl_types.h"

#include <vector>
#include <iostream>

//number of bits to use for comm id
constexpr const size_t MLSL_COMM_ID_BITS = 16;
//max number of the comm ids
constexpr const size_t MLSL_COMM_ID_MAX = (1ull << MLSL_COMM_ID_BITS) - 1;

//number of bits to use for sched id (per each communicator)
constexpr const size_t MLSL_COMM_SCHED_ID_BITS = 16;
//max number of the schedules per comm
constexpr const size_t MLSL_COMM_SCHED_ID_MAX = (1ull << MLSL_COMM_SCHED_ID_BITS) - 1;

constexpr const size_t MLSL_SCHED_ID_SHIFT = 16;

/*********************************************************************************
 *  atl tag                                                                      *
 * *******************************************************************************
 * 01234567 01234567 01234567 01234567 | 01234567 01234567 | 01234567 01234567   |
 *                                     |                                         |
 *             global rank             | sched_id(per comm)|      comm_id        |
 *********************************************************************************/
//number of bits used to construct atl tag from schedule id
constexpr const size_t MLSL_ATL_COMM_SCHEDULE_TAG_BITS = 32;
//mask to filter schedule number
constexpr const size_t MLSL_ATL_COMM_SCHED_TAG_MASK = (1ull << MLSL_ATL_COMM_SCHEDULE_TAG_BITS) - 1;
//number of bits used to construct atl tag from rank
constexpr const size_t MLSL_ATL_COMM_RANK_SHIFT = 32;

class comm_id_storage
{
public:
    comm_id_storage()
    {
        free_ids.resize(MLSL_COMM_ID_MAX, true);
    }
    mlsl_status_t acquire_id(mlsl_comm_id_t& result)
    {
        //search from the current position till the end
        for (size_t i = last_used; i < MLSL_COMM_ID_MAX; ++i)
        {
            if (free_ids[i])
            {
                free_ids[i] = false;
                last_used = i;
                result = i;
                MLSL_LOG(DEBUG, "Found free comm id %hu", result);
                return mlsl_status_success;
            }
        }

        //if we didn't exit from the method than there are no free ids in range [last_used:MLSL_COMM_MAX)
        //need to repeat from the beginning of the ids space [0:last_used)
        for (size_t i = 0; i < last_used; ++i)
        {
            if (free_ids[i])
            {
                free_ids[i] = false;
                last_used = i;
                result = i;
                MLSL_LOG(DEBUG, "Found free comm id %hu", result);
                return mlsl_status_success;
            }
        }

        //nothing was found
        MLSL_LOG(INFO, "No free comm ids were found");
        return mlsl_status_out_of_resource;
    }

    mlsl_status_t release_id(mlsl_comm_id_t id)
    {
        if (free_ids[id])
        {
            MLSL_LOG(ERROR, "Attempt to relase not acquired id %hu", id);
            return mlsl_status_invalid_arguments;
        }
        free_ids[id] = true;
        last_used = id;

        return mlsl_status_success;
    }

private:
    size_t last_used = 0;
    std::vector<bool> free_ids;
};

static comm_id_storage id_storage;

mlsl_status_t mlsl_comm_id_acquire(mlsl_comm_id_t* out_id)
{
    MLSL_ASSERT(out_id);
    return id_storage.acquire_id(*out_id);
}

mlsl_status_t mlsl_comm_id_release(mlsl_comm_id_t id)
{
    return id_storage.release_id(id);
}

mlsl_sched_id_t mlsl_comm_get_sched_id(mlsl_comm *comm)
{
    MLSL_ASSERT(comm);

    mlsl_sched_id_t id = comm->next_sched_id;

    ++comm->next_sched_id;

    if (comm->next_sched_id == MLSL_COMM_SCHED_ID_MAX)
    {
        /* wrap the sched numbers around to the start */
        comm->next_sched_id = 0;
    }

    MLSL_LOG(DEBUG, "sched_id %u, comm_id %hu, next sched_id %hu", id, comm->comm_id,
             comm->next_sched_id);

    return id;
}

mlsl_atl_comm_tag_t mlsl_create_atl_tag(mlsl_comm_id_t comm_id, mlsl_sched_id_t sched_id, size_t rank)
{
    mlsl_atl_comm_tag_t tag = comm_id;
    tag |= sched_id << MLSL_SCHED_ID_SHIFT;
    tag &= MLSL_ATL_COMM_SCHED_TAG_MASK;
    tag |= rank << MLSL_ATL_COMM_RANK_SHIFT;

    MLSL_LOG(DEBUG, "sched_id %u, rank %zu, tag %llu", sched_id, rank, tag);
    return tag;
}

